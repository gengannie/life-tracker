#include "src/stats.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "absl/strings/str_format.h"
#include "absl/time/time.h"

namespace life_tracker {

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

  double variance = 0.0;
  for (const auto& sample : samples) {
    const double diff = static_cast<double>(sample.mood) - summary.average_mood;
    variance += diff * diff;
  }
  variance /= summary.count;
  summary.stddev = std::sqrt(variance);
  return summary;
}

std::vector<DayMood> CollectRecentSamples(const std::vector<Entry>& entries, int days,
                                          absl::CivilDay today) {
  if (days <= 0) {
    throw std::runtime_error("--days must be positive.");
  }

  const absl::CivilDay cutoff = today - (days - 1);

  std::vector<DayMood> samples;
  samples.reserve(entries.size());
  for (const auto& entry : entries) {
    const absl::CivilDay entry_day = ParseCivilDay(entry.date);
    if (entry_day < cutoff) continue;
    samples.push_back({entry_day, entry.mood});
  }

  std::sort(samples.begin(), samples.end(),
            [](const DayMood& a, const DayMood& b) { return a.day < b.day; });

  return samples;
}

StreakStats ComputeStreaks(const std::vector<Entry>& entries, absl::CivilDay today) {
  StreakStats streaks;
  if (entries.empty()) return streaks;

  std::vector<absl::CivilDay> days;
  days.reserve(entries.size());
  for (const auto& entry : entries) {
    days.push_back(ParseCivilDay(entry.date));
  }
  std::sort(days.begin(), days.end());
  days.erase(std::unique(days.begin(), days.end()), days.end());

  int longest = 0;
  int current_run = 0;
  absl::CivilDay prev = days.front() - 1;
  for (const auto& day : days) {
    if (day == prev + 1) {
      ++current_run;
    } else {
      current_run = 1;
    }
    longest = std::max(longest, current_run);
    prev = day;
  }

  int current = 0;
  if (!days.empty()) {
    const absl::CivilDay latest = days.back();
    if (latest == today || latest == today - 1) {
      current = 1;
      for (int i = static_cast<int>(days.size()) - 2; i >= 0; --i) {
        if (days[i] == days[i + 1] - 1) {
          ++current;
        } else {
          break;
        }
      }
    }
  }

  streaks.current_streak = current;
  streaks.longest_streak = longest;
  return streaks;
}

}  // namespace life_tracker
