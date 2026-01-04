#include "src/stats.h"

#include <cmath>
#include <vector>

#include "gtest/gtest.h"

namespace life_tracker {
namespace {

Entry MakeEntry(const std::string& date, int mood) {
  Entry e;
  e.date = date;
  e.mood = mood;
  e.note = "";
  return e;
}

TEST(ComputeSummaryTest, ComputesAverageAndStdDev) {
  std::vector<DayMood> samples = {
      {absl::CivilDay(2026, 1, 1), 50},
      {absl::CivilDay(2026, 1, 2), 70},
      {absl::CivilDay(2026, 1, 3), 90},
  };

  const SummaryStats summary = ComputeSummary(samples);

  EXPECT_TRUE(summary.has_data);
  EXPECT_EQ(summary.count, 3);
  EXPECT_NEAR(summary.average_mood, 70.0, 1e-6);
  EXPECT_NEAR(summary.stddev, std::sqrt(266.6666667), 1e-5);
  EXPECT_EQ(summary.best.day, absl::CivilDay(2026, 1, 3));
  EXPECT_EQ(summary.best.mood, 90);
  EXPECT_EQ(summary.worst.day, absl::CivilDay(2026, 1, 1));
  EXPECT_EQ(summary.worst.mood, 50);
}

TEST(CollectRecentSamplesTest, FiltersByDays) {
  std::vector<Entry> entries = {
      MakeEntry("2026-01-01", 50),
      MakeEntry("2026-01-02", 60),
      MakeEntry("2026-01-05", 70),
  };

  const absl::CivilDay today(2026, 1, 5);

  const std::vector<DayMood> samples = CollectRecentSamples(entries, 3, today);
  ASSERT_EQ(samples.size(), 1);
  EXPECT_EQ(samples[0].day, absl::CivilDay(2026, 1, 5));
  EXPECT_EQ(samples[0].mood, 70);
}

TEST(ComputeStreaksTest, HandlesCurrentAndLongest) {
  std::vector<Entry> entries = {
      MakeEntry("2026-01-01", 60), MakeEntry("2026-01-02", 65), MakeEntry("2026-01-03", 70),
      MakeEntry("2026-01-05", 80), MakeEntry("2026-01-06", 70),
  };

  const absl::CivilDay today(2026, 1, 6);
  const StreakStats streaks = ComputeStreaks(entries, today);

  EXPECT_EQ(streaks.current_streak, 2);  // 5th and 6th
  EXPECT_EQ(streaks.longest_streak, 3);  // 1st-3rd
}

TEST(ComputeStreaksTest, ZeroWhenNoRecentDay) {
  std::vector<Entry> entries = {
      MakeEntry("2026-01-01", 60),
      MakeEntry("2026-01-10", 70),
  };

  const absl::CivilDay today(2026, 1, 20);
  const StreakStats streaks = ComputeStreaks(entries, today);

  EXPECT_EQ(streaks.current_streak, 0);
  EXPECT_EQ(streaks.longest_streak, 1);
}

}  // namespace
}  // namespace life_tracker
