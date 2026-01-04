#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"
#include "absl/time/time.h"
#include "src/path_utils.h"
#include "src/tracker.h"

ABSL_FLAG(int, mood, 0, "Mood rating 1..5");
ABSL_FLAG(std::string, note, "", "Free-form note");
ABSL_FLAG(std::string, date, "", "Date in YYYY-MM-DD (default: today)");
ABSL_FLAG(std::string, data_path, "data/entries.csv", "Path to entries CSV");
ABSL_FLAG(int, days, 7, "Number of days to include in reports");
ABSL_FLAG(std::string, out, "report.html", "Where to write generated reports");

namespace life_tracker {
namespace {

struct DayMood {
  absl::CivilDay day;
  int mood;
};

struct SummaryStats {
  bool has_data = false;
  int count = 0;
  double average_mood = 0.0;
  DayMood best;
  DayMood worst;
};

absl::CivilDay ParseCivilDay(const std::string& date_str) {
  absl::Time timestamp;
  if (!absl::ParseTime("%Y-%m-%d", date_str, absl::UTCTimeZone(), &timestamp, nullptr)) {
    throw std::runtime_error("Invalid date in data: " + date_str);
  }
  return absl::ToCivilDay(timestamp, absl::UTCTimeZone());
}

SummaryStats ComputeSummary(const std::vector<DayMood>& samples) {
  SummaryStats summary;
  if (samples.empty()) return summary;

  summary.has_data = true;
  summary.count = static_cast<int>(samples.size());
  summary.best = samples.front();
  summary.worst = samples.front();

  int total = 0;
  for (const auto& sample : samples) {
    total += sample.mood;
    if (sample.mood > summary.best.mood ||
        (sample.mood == summary.best.mood && sample.day < summary.best.day)) {
      summary.best = sample;
    }
    if (sample.mood < summary.worst.mood ||
        (sample.mood == summary.worst.mood && sample.day < summary.worst.day)) {
      summary.worst = sample;
    }
  }

  summary.average_mood = static_cast<double>(total) / summary.count;
  return summary;
}

std::string RenderSvg(const std::vector<DayMood>& samples) {
  if (samples.empty()) {
    return "<div class=\"empty\">No data to chart.</div>";
  }

  const int width = 720;
  const int height = 360;
  const int padding = 48;
  const int plot_width = width - 2 * padding;
  const int plot_height = height - 2 * padding;

  const double x_step =
      samples.size() > 1 ? static_cast<double>(plot_width) / (samples.size() - 1) : 0.0;

  std::ostringstream points;
  for (size_t i = 0; i < samples.size(); ++i) {
    const double x = padding + x_step * i;
    // Mood range 1..5 mapped top (5) to bottom (1).
    const double y = padding + (5 - samples[i].mood) * (plot_height / 4.0);
    points << x << "," << y;
    if (i + 1 < samples.size()) points << " ";
  }

  std::ostringstream circles;
  for (size_t i = 0; i < samples.size(); ++i) {
    const double x = padding + x_step * i;
    const double y = padding + (5 - samples[i].mood) * (plot_height / 4.0);
    circles << "<circle cx=\"" << x << "\" cy=\"" << y
            << "\" r=\"5\" fill=\"#2563eb\" stroke=\"white\" stroke-width=\"2\"></circle>";
  }

  std::ostringstream svg;
  svg << "<svg width=\"" << width << "\" height=\"" << height << "\" viewBox=\"0 0 " << width << " "
      << height << "\" role=\"img\" "
      << "aria-label=\"Mood over time\">";
  svg << "<rect x=\"0\" y=\"0\" width=\"" << width << "\" height=\"" << height
      << "\" fill=\"#f8fafc\" />";
  svg << "<polyline fill=\"none\" stroke=\"#2563eb\" stroke-width=\"3\" points=\"" << points.str()
      << "\"></polyline>";
  svg << circles.str();
  svg << "<text x=\"" << padding << "\" y=\"" << height - padding / 3
      << "\" fill=\"#475569\" font-family=\"Helvetica, Arial, sans-serif\" "
         "font-size=\"12\">Older</text>";
  svg << "<text x=\"" << width - padding << "\" y=\"" << height - padding / 3
      << "\" fill=\"#475569\" font-family=\"Helvetica, Arial, sans-serif\" font-size=\"12\" "
      << "text-anchor=\"end\">Newer</text>";
  svg << "</svg>";
  return svg.str();
}

std::string BuildReportHtml(const std::vector<DayMood>& samples, const SummaryStats& summary,
                            int days) {
  std::ostringstream html;
  html << "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>Life Tracker Report</title>";
  html << "<style>"
       << "body{font-family:Helvetica,Arial,sans-serif;background:#0f172a;color:#e2e8f0;"
          "margin:0;padding:32px;}"
       << "h1{margin:0 0 8px 0;font-size:28px;}"
       << "p.lead{margin:0 0 24px 0;color:#cbd5e1;}"
       << ".cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));"
          "gap:16px;margin-bottom:24px;}"
       << ".card{background:#1e293b;border:1px solid #334155;border-radius:12px;padding:16px;"
          "box-shadow:0 10px 30px rgba(0,0,0,0.3);}"
       << ".label{font-size:12px;letter-spacing:0.08em;text-transform:uppercase;color:#94a3b8;"
          "margin-bottom:6px;display:block;}"
       << ".value{font-size:22px;font-weight:700;}"
       << ".chart{background:#fff;border-radius:12px;border:1px solid #e2e8f0;"
          "padding:12px;}"
       << ".chart h2{color:#0f172a;margin:0 0 8px 0;}"
       << ".empty{color:#334155;font-style:italic;}"
       << "</style></head><body>";
  html << "<h1>Life Tracker Report</h1>";
  html << "<p class=\"lead\">Last " << days << " day";
  if (days != 1) html << "s";
  html << " of mood entries.</p>";

  html << "<div class=\"cards\">";
  html << "<div class=\"card\"><span class=\"label\">Entries</span><div class=\"value\">"
       << summary.count << "</div></div>";
  html << "<div class=\"card\"><span class=\"label\">Average Mood</span><div class=\"value\">";
  if (summary.has_data) {
    html << absl::StrFormat("%.2f", summary.average_mood);
  } else {
    html << "n/a";
  }
  html << "</div></div>";
  html << "<div class=\"card\"><span class=\"label\">Best Day</span><div class=\"value\">";
  if (summary.has_data) {
    html << absl::FormatCivilTime(summary.best.day) << " (" << summary.best.mood << ")";
  } else {
    html << "n/a";
  }
  html << "</div></div>";
  html << "<div class=\"card\"><span class=\"label\">Toughest Day</span><div class=\"value\">";
  if (summary.has_data) {
    html << absl::FormatCivilTime(summary.worst.day) << " (" << summary.worst.mood << ")";
  } else {
    html << "n/a";
  }
  html << "</div></div>";
  html << "</div>";

  html << "<div class=\"chart\"><h2>Mood Over Time</h2>" << RenderSvg(samples) << "</div>";

  html << "</body></html>";
  return html.str();
}

std::string TodayIsoDate() {
  std::time_t t = std::time(nullptr);
  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &t);
#else
  localtime_r(&t, &tm);
#endif
  char buf[11];
  std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
  return std::string(buf);
}

void PrintUsage() {
  std::cerr << "Usage:\n"
            << "  life add --mood=3 --note=\"text\" [--date=YYYY-MM-DD]\n"
            << "  life list\n"
            << "  life report [--days=N] [--out=PATH]\n"
            << "Flags:\n"
            << "  --data_path=PATH   Where to store entries (default: data/entries.csv)\n"
            << "  --days=N           Number of days to include in reports (default: 7)\n"
            << "  --out=PATH         Where to write reports (default: report.html)\n";
}

int RunAdd(const std::vector<std::string>& args) {
  (void)args;  // subcommand-specific positional args currently unused.

  const int mood = absl::GetFlag(FLAGS_mood);
  const std::string note = absl::GetFlag(FLAGS_note);
  std::string date = absl::GetFlag(FLAGS_date);
  if (date.empty()) date = TodayIsoDate();

  const std::string data_path = ResolveDataPath(absl::GetFlag(FLAGS_data_path));

  Tracker tracker(data_path);
  tracker.Load();

  Entry e;
  e.date = date;
  e.mood = mood;
  e.note = note;

  tracker.Add(e);

  std::cout << "Added: " << e.date << " mood=" << e.mood << " note=\"" << e.note << "\"\n";
  return 0;
}

int RunList(const std::vector<std::string>& args) {
  (void)args;

  const std::string data_path = ResolveDataPath(absl::GetFlag(FLAGS_data_path));

  Tracker tracker(data_path);
  tracker.Load();

  const auto& entries = tracker.Entries();
  if (entries.empty()) {
    std::cout << "No entries yet.\n";
    return 0;
  }

  // Newest last in file; print newest-first.
  for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
    std::cout << it->date << "  mood=" << it->mood << "  " << it->note << "\n";
  }
  return 0;
}

int RunReport(const std::vector<std::string>& args) {
  (void)args;

  const int days = absl::GetFlag(FLAGS_days);
  if (days <= 0) {
    throw std::runtime_error("--days must be positive.");
  }

  const std::string data_path = ResolveDataPath(absl::GetFlag(FLAGS_data_path));
  const std::string out_path = ResolveDataPath(absl::GetFlag(FLAGS_out));

  Tracker tracker(data_path);
  tracker.Load();

  const absl::CivilDay today = absl::ToCivilDay(absl::Now(), absl::UTCTimeZone());
  const absl::CivilDay cutoff = today - (days - 1);

  std::vector<DayMood> samples;
  for (const auto& entry : tracker.Entries()) {
    const absl::CivilDay entry_day = ParseCivilDay(entry.date);
    if (entry_day < cutoff) continue;
    samples.push_back({entry_day, entry.mood});
  }

  std::sort(samples.begin(), samples.end(),
            [](const DayMood& a, const DayMood& b) { return a.day < b.day; });

  const SummaryStats summary = ComputeSummary(samples);
  const std::string html = BuildReportHtml(samples, summary, days);

  std::filesystem::path path(out_path);
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream out(out_path, std::ios::out | std::ios::trunc);
  if (!out.is_open()) {
    throw std::runtime_error("Failed to open report file for writing: " + out_path);
  }
  out << html;
  out.close();

  std::cout << "Report written to " << out_path << "\n";
  return 0;
}

}  // namespace
}  // namespace life_tracker

int main(int argc, char* argv[]) {
  std::vector<char*> remaining = absl::ParseCommandLine(argc, argv);

  if (remaining.size() < 2) {
    life_tracker::PrintUsage();
    return 1;
  }

  const std::string command = remaining[1];
  std::vector<std::string> positional;
  for (size_t i = 2; i < remaining.size(); ++i) {
    positional.emplace_back(remaining[i]);
  }

  try {
    if (command == "add") {
      return life_tracker::RunAdd(positional);
    }
    if (command == "list") {
      return life_tracker::RunList(positional);
    }
    if (command == "report") {
      return life_tracker::RunReport(positional);
    }
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 2;
  }

  life_tracker::PrintUsage();
  return 1;
}
