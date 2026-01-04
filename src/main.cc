#include <algorithm>
#include <cmath>
#include <cstdlib>
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
#include "src/stats.h"
#include "src/tracker.h"

ABSL_FLAG(int, mood, 0, "Mood rating 1..100");
ABSL_FLAG(std::string, note, "", "Free-form note");
ABSL_FLAG(std::string, date, "", "Date in YYYY-MM-DD (default: today)");
ABSL_FLAG(std::string, data_path, "data/entries.csv", "Path to entries CSV");
ABSL_FLAG(int, days, 7, "Number of days to include in reports");
ABSL_FLAG(std::string, out, "report.html",
          "Where to write generated reports/exports/dashboard data");
ABSL_FLAG(std::string, format, "json", "Export format (currently: json)");
ABSL_FLAG(bool, open, true, "Whether to open the dashboard URL after export");
ABSL_FLAG(std::string, url, "http://localhost:3000", "Dashboard URL to open when --open=true");

namespace life_tracker {
namespace {

std::string RenderSvg(const std::vector<DayMood>& samples) {
  if (samples.empty()) {
    return "<div class=\"empty\">No data to chart.</div>";
  }

  const int min_mood = 1;
  const int max_mood = 100;
  const double mood_range = static_cast<double>(max_mood - min_mood);

  const int width = 720;
  const int height = 360;
  const int padding = 48;
  const int plot_width = width - 2 * padding;
  const int plot_height = height - 2 * padding;

  const double x_step =
      samples.size() > 1 ? static_cast<double>(plot_width) / (samples.size() - 1) : 0.0;

  auto MoodToY = [&](int mood) {
    const double clamped = std::min(std::max(mood, min_mood), max_mood);
    const double normalized = (max_mood - clamped) / mood_range;  // 0 at max, 1 at min.
    return padding + normalized * plot_height;
  };

  std::ostringstream points;
  for (size_t i = 0; i < samples.size(); ++i) {
    const double x = padding + x_step * i;
    const double y = MoodToY(samples[i].mood);
    points << x << "," << y;
    if (i + 1 < samples.size()) points << " ";
  }

  std::ostringstream circles;
  for (size_t i = 0; i < samples.size(); ++i) {
    const double x = padding + x_step * i;
    const double y = MoodToY(samples[i].mood);
    circles << "<circle cx=\"" << x << "\" cy=\"" << y
            << "\" r=\"5\" fill=\"#2563eb\" stroke=\"white\" stroke-width=\"2\"></circle>";
  }

  std::ostringstream svg;
  svg << "<svg width=\"" << width << "\" height=\"" << height << "\" viewBox=\"0 0 " << width << " "
      << height << "\" role=\"img\" "
      << "aria-label=\"Mood over time (1-100)\">";
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
  svg << "<text x=\"" << padding << "\" y=\"" << padding / 1.8
      << "\" fill=\"#475569\" font-family=\"Helvetica, Arial, sans-serif\" font-size=\"12\">Mood "
         "(1-100)</text>";
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
            << "  life add --mood=42 --note=\"text\" [--date=YYYY-MM-DD]\n"
            << "  life list\n"
            << "  life summary [--days=N]\n"
            << "  life report [--days=N] [--out=PATH]\n"
            << "  life export [--format=json] [--out=PATH]\n"
            << "  life dashboard [--out=PATH] [--open=true] [--url=URL]\n"
            << "Flags:\n"
            << "  --data_path=PATH   Where to store entries (default: data/entries.csv)\n"
            << "  --days=N           Number of days to include in reports (default: 7)\n"
            << "  --out=PATH         Where to write reports/exports (default: report.html)\n"
            << "  --format=FORMAT    Export format (default: json)\n"
            << "  --open=true/false  Open dashboard URL after exporting data (default: true)\n"
            << "  --url=URL          Dashboard URL to open when --open=true (default: "
               "http://localhost:3000)\n";
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
  const std::string data_path = ResolveDataPath(absl::GetFlag(FLAGS_data_path));
  const std::string out_path = ResolveDataPath(absl::GetFlag(FLAGS_out));

  Tracker tracker(data_path);
  tracker.Load();

  const std::vector<DayMood> samples = CollectRecentSamples(
      tracker.Entries(), days, absl::ToCivilDay(absl::Now(), absl::UTCTimeZone()));

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

bool OpenDashboardUrl(const std::string& url) {
#if defined(_WIN32)
  const std::string cmd = "start \"\" \"" + url + "\"";
#elif defined(__APPLE__)
  const std::string cmd = "open \"" + url + "\"";
#else
  const std::string cmd = "xdg-open \"" + url + "\"";
#endif
  const int rc = std::system(cmd.c_str());
  return rc == 0;
}

std::string JsonEscape(const std::string& s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (char c : s) {
    switch (c) {
      case '\"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\b':
        out += "\\b";
        break;
      case '\f':
        out += "\\f";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          out += absl::StrFormat("\\u%04x", static_cast<int>(static_cast<unsigned char>(c)));
        } else {
          out.push_back(c);
        }
    }
  }
  return out;
}

std::string ExportEntriesToJson(const std::string& out_flag, const std::string& default_out,
                                int summary_days, absl::CivilDay today) {
  const std::string format = absl::GetFlag(FLAGS_format);
  if (format != "json") {
    throw std::runtime_error("Unsupported export format: " + format);
  }

  const std::string resolved_out_flag = (out_flag == "report.html") ? default_out : out_flag;

  const std::string data_path = ResolveDataPath(absl::GetFlag(FLAGS_data_path));
  const std::string out_path = ResolveDataPath(resolved_out_flag);

  Tracker tracker(data_path);
  tracker.Load();

  const auto& entries = tracker.Entries();
  const std::vector<DayMood> samples = CollectRecentSamples(entries, summary_days, today);
  const SummaryStats summary = ComputeSummary(samples);
  const StreakStats streak = ComputeStreaks(entries, today);

  std::filesystem::path path(out_path);
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream out(out_path, std::ios::out | std::ios::trunc);
  if (!out.is_open()) {
    throw std::runtime_error("Failed to open export file for writing: " + out_path);
  }

  out << "{\n";
  out << "  \"meta\": {\"generated_at\":\""
      << JsonEscape(absl::FormatTime(absl::Now(), absl::UTCTimeZone()))
      << "\", \"days\":" << summary_days << "},\n";

  out << "  \"summary\": {\n";
  out << "    \"has_data\":" << (summary.has_data ? "true" : "false") << ",\n";
  out << "    \"count\":" << summary.count << ",\n";
  out << "    \"average_mood\":" << absl::StrFormat("%.1f", summary.average_mood) << ",\n";
  out << "    \"stddev\":" << absl::StrFormat("%.1f", summary.stddev) << ",\n";
  out << "    \"best\":";
  if (summary.has_data) {
    out << "{\"date\":\"" << JsonEscape(absl::FormatCivilTime(summary.best.day))
        << "\",\"mood\":" << summary.best.mood << "}";
  } else {
    out << "null";
  }
  out << ",\n";
  out << "    \"worst\":";
  if (summary.has_data) {
    out << "{\"date\":\"" << JsonEscape(absl::FormatCivilTime(summary.worst.day))
        << "\",\"mood\":" << summary.worst.mood << "}";
  } else {
    out << "null";
  }
  out << "\n  },\n";

  out << "  \"streak\": {\"current\":" << streak.current_streak
      << ", \"longest\":" << streak.longest_streak << "},\n";

  out << "  \"entries\": [";
  for (size_t i = 0; i < entries.size(); ++i) {
    const Entry& e = entries[i];
    out << "\n    {\"date\":\"" << JsonEscape(e.date) << "\",\"mood\":" << e.mood << ",\"note\":\""
        << JsonEscape(e.note) << "\"}";
    if (i + 1 < entries.size()) out << ",";
  }
  if (!entries.empty()) out << "\n";
  out << "  ]\n";
  out << "}\n";
  out.close();

  return out_path;
}

int RunExport(const std::vector<std::string>& args) {
  (void)args;

  const std::string out_flag = absl::GetFlag(FLAGS_out);
  const absl::CivilDay today = absl::ToCivilDay(absl::Now(), absl::UTCTimeZone());
  const int summary_days = absl::GetFlag(FLAGS_days);
  const std::string out_path = ExportEntriesToJson(out_flag, "export.json", summary_days, today);

  std::cout << "Export written to " << out_path << "\n";
  return 0;
}

int RunDashboard(const std::vector<std::string>& args) {
  (void)args;

  const std::string out_flag = absl::GetFlag(FLAGS_out);
  const absl::CivilDay today = absl::ToCivilDay(absl::Now(), absl::UTCTimeZone());
  const int summary_days = absl::GetFlag(FLAGS_days);
  const std::string out_path =
      ExportEntriesToJson(out_flag, "web/data/entries.json", summary_days, today);

  const bool should_open = absl::GetFlag(FLAGS_open);
  const std::string url = absl::GetFlag(FLAGS_url);

  if (should_open) {
    if (!OpenDashboardUrl(url)) {
      std::cerr << "Dashboard data written to " << out_path
                << " but opening the browser failed. Open manually: " << url << "\n";
    }
  }

  std::cout << "Dashboard data written to " << out_path << ". Dashboard URL: " << url << "\n";
  return 0;
}

int RunSummary(const std::vector<std::string>& args) {
  (void)args;

  const int days = absl::GetFlag(FLAGS_days);
  const std::string data_path = ResolveDataPath(absl::GetFlag(FLAGS_data_path));

  Tracker tracker(data_path);
  tracker.Load();

  const std::vector<DayMood> samples = CollectRecentSamples(
      tracker.Entries(), days, absl::ToCivilDay(absl::Now(), absl::UTCTimeZone()));
  if (samples.empty()) {
    std::cout << "No entries in the last " << days << " day";
    if (days != 1) std::cout << "s";
    std::cout << ".\n";
    return 0;
  }

  const SummaryStats summary = ComputeSummary(samples);

  std::cout << "Entries: " << summary.count << "\n";
  std::cout << "Average mood: " << absl::StrFormat("%.1f", summary.average_mood) << "\n";
  std::cout << "Best day: " << absl::FormatCivilTime(summary.best.day) << " (" << summary.best.mood
            << ")\n";
  std::cout << "Worst day: " << absl::FormatCivilTime(summary.worst.day) << " ("
            << summary.worst.mood << ")\n";
  std::cout << "Mood volatility (std dev): " << absl::StrFormat("%.1f", summary.stddev) << "\n";

  return 0;
}

int RunStreak(const std::vector<std::string>& args) {
  (void)args;

  const std::string data_path = ResolveDataPath(absl::GetFlag(FLAGS_data_path));

  Tracker tracker(data_path);
  tracker.Load();

  const StreakStats stats =
      ComputeStreaks(tracker.Entries(), absl::ToCivilDay(absl::Now(), absl::UTCTimeZone()));

  std::cout << "Current streak: " << stats.current_streak << " day";
  if (stats.current_streak != 1) std::cout << "s";
  std::cout << "\n";
  std::cout << "Longest streak: " << stats.longest_streak << " day";
  if (stats.longest_streak != 1) std::cout << "s";
  std::cout << "\n";

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
    if (command == "summary") {
      return life_tracker::RunSummary(positional);
    }
    if (command == "report") {
      return life_tracker::RunReport(positional);
    }
    if (command == "export") {
      return life_tracker::RunExport(positional);
    }
    if (command == "dashboard") {
      return life_tracker::RunDashboard(positional);
    }
    if (command == "streak") {
      return life_tracker::RunStreak(positional);
    }
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 2;
  }

  life_tracker::PrintUsage();
  return 1;
}
