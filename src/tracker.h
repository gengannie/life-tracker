#ifndef LIFE_TRACKER_TRACKER_H_
#define LIFE_TRACKER_TRACKER_H_

#include <string>
#include <vector>

#include "src/entry.h"

namespace life_tracker {

class Tracker {
 public:
  explicit Tracker(std::string data_path);

  void Load();
  void Add(const Entry& entry);
  const std::vector<Entry>& Entries() const;

 private:
  void AppendToDisk(const Entry& entry) const;

  std::string data_path_;
  std::vector<Entry> entries_;
};

}  // namespace life_tracker

#endif  // LIFE_TRACKER_TRACKER_H_
