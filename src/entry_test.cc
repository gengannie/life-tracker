#include "src/entry.h"

#include "gtest/gtest.h"

namespace life_tracker {

TEST(EntryTest, CsvRoundTripWithCommaAndQuotes) {
  Entry e;
  e.date = "2026-01-03";
  e.mood = 5;
  e.note = "hello, \"world\"";

  const std::string csv = e.ToCsv();
  Entry parsed = Entry::FromCsvLine(csv);

  EXPECT_EQ(parsed.date, e.date);
  EXPECT_EQ(parsed.mood, e.mood);
  EXPECT_EQ(parsed.note, e.note);
}

}  // namespace life_tracker
