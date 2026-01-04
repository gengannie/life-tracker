#ifndef LIFE_TRACKER_STATS_H_
#define LIFE_TRACKER_STATS_H_

#include <vector>

#include "absl/time/time.h"
#include "src/entry.h"

namespace life_tracker {

struct DayMood {
  absl::CivilDay day;
  int mood;
};

struct SummaryStats {
  bool has_data = false;
  int count = 0;
  double average_mood = 0.0;
  double stddev = 0.0;
  DayMood best;
  DayMood worst;
};

struct StreakStats {
  int current_streak = 0;
  int longest_streak = 0;
};

SummaryStats ComputeSummary(const std::vector<DayMood>& samples);

std::vector<DayMood> CollectRecentSamples(const std::vector<Entry>& entries, int days,
                                          absl::CivilDay today);

StreakStats ComputeStreaks(const std::vector<Entry>& entries, absl::CivilDay today);

absl::CivilDay ParseCivilDay(const std::string& date_str);

}  // namespace life_tracker

#endif  // LIFE_TRACKER_STATS_H_
