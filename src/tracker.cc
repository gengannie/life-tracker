#include "src/tracker.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace life_tracker {
namespace fs = std::filesystem;

Tracker::Tracker(std::string data_path) : data_path_(std::move(data_path)) {}

void Tracker::Load() {
  entries_.clear();

  std::ifstream in(data_path_);
  if (!in.is_open()) return;  // No file yet is fine.

  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    entries_.push_back(Entry::FromCsvLine(line));
  }
}

void Tracker::Add(const Entry& entry) {
  // Basic validation.
  if (entry.mood < 1 || entry.mood > 5) {
    throw std::runtime_error("Mood must be between 1 and 5.");
  }
  if (entry.date.size() != 10) {
    throw std::runtime_error("Date must be YYYY-MM-DD.");
  }

  entries_.push_back(entry);
  AppendToDisk(entry);
}

const std::vector<Entry>& Tracker::Entries() const { return entries_; }

void Tracker::AppendToDisk(const Entry& entry) const {
  fs::path path(data_path_);
  if (path.has_parent_path()) {
    fs::create_directories(path.parent_path());
  }

  std::ofstream out(data_path_, std::ios::app);
  if (!out.is_open()) {
    throw std::runtime_error("Failed to open data file for writing: " + data_path_);
  }
  out << entry.ToCsv() << "\n";
}

}  // namespace life_tracker
