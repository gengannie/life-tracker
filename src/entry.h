#ifndef LIFE_TRACKER_ENTRY_H_
#define LIFE_TRACKER_ENTRY_H_

#include <string>

namespace life_tracker {

struct Entry {
  std::string date;  // YYYY-MM-DD
  int mood = 0;      // 1..5
  std::string note;

  std::string ToCsv() const;
  static Entry FromCsvLine(const std::string& line);
};

}  // namespace life_tracker

#endif  // LIFE_TRACKER_ENTRY_H_
