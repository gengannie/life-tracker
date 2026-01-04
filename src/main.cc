#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_join.h"
#include "src/tracker.h"

ABSL_FLAG(int, mood, 0, "Mood rating 1..5");
ABSL_FLAG(std::string, note, "", "Free-form note");
ABSL_FLAG(std::string, date, "", "Date in YYYY-MM-DD (default: today)");
ABSL_FLAG(std::string, data_path, "data/entries.csv", "Path to entries CSV");

namespace life_tracker {
namespace {

std::string TodayIsoDate() {
  std::time_t t = std::time(nullptr);
  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &t);
#else
  localtime_r(&t, &tm);
#endif
  char buf[11];
  std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
  return std::string(buf);
}

void PrintUsage() {
  std::cerr
      << "Usage:\n"
      << "  life add --mood=3 --note=\"text\" [--date=YYYY-MM-DD]\n"
      << "  life list\n"
      << "Flags:\n"
      << "  --data_path=PATH   Where to store entries (default: data/entries.csv)\n";
}

int RunAdd(const std::vector<std::string>& args) {
  (void)args;  // subcommand-specific positional args currently unused.

  const int mood = absl::GetFlag(FLAGS_mood);
  const std::string note = absl::GetFlag(FLAGS_note);
  std::string date = absl::GetFlag(FLAGS_date);
  if (date.empty()) date = TodayIsoDate();

  Tracker tracker(absl::GetFlag(FLAGS_data_path));
  tracker.Load();

  Entry e;
  e.date = date;
  e.mood = mood;
  e.note = note;

  tracker.Add(e);

  std::cout << "Added: " << e.date << " mood=" << e.mood << " note=\"" << e.note
            << "\"\n";
  return 0;
}

int RunList(const std::vector<std::string>& args) {
  (void)args;

  Tracker tracker(absl::GetFlag(FLAGS_data_path));
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
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 2;
  }

  life_tracker::PrintUsage();
  return 1;
}
