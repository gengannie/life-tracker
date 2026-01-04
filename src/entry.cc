#include "src/entry.h"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>

namespace life_tracker {
namespace {

std::string TrimTrailingCarriageReturn(const std::string& s) {
  if (!s.empty() && s.back() == '\r') return s.substr(0, s.size() - 1);
  return s;
}

std::string EscapeCsvField(const std::string& s) {
  bool needs_quotes = false;
  for (char c : s) {
    if (c == ',' || c == '"' || c == '\n' || c == '\r') {
      needs_quotes = true;
      break;
    }
  }
  if (!needs_quotes) return s;

  std::string out;
  out.reserve(s.size() + 2);
  out.push_back('"');
  for (char c : s) {
    if (c == '"') {
      out += "\"\"";
    } else {
      out.push_back(c);
    }
  }
  out.push_back('"');
  return out;
}

std::string ReadCsvField(const std::string& line, size_t* i) {
  std::string out;
  if (*i >= line.size()) return out;

  if (line[*i] == '"') {
    ++(*i);
    while (*i < line.size()) {
      if (line[*i] == '"') {
        if (*i + 1 < line.size() && line[*i + 1] == '"') {
          out.push_back('"');
          *i += 2;
        } else {
          ++(*i);
          break;
        }
      } else {
        out.push_back(line[*i]);
        ++(*i);
      }
    }
    if (*i < line.size() && line[*i] == ',') ++(*i);
    return out;
  }

  while (*i < line.size() && line[*i] != ',') {
    out.push_back(line[*i]);
    ++(*i);
  }
  if (*i < line.size() && line[*i] == ',') ++(*i);
  return out;
}

}  // namespace

std::string Entry::ToCsv() const {
  std::ostringstream oss;
  oss << date << "," << mood << "," << EscapeCsvField(note);
  return oss.str();
}

Entry Entry::FromCsvLine(const std::string& line_in) {
  const std::string line = TrimTrailingCarriageReturn(line_in);

  size_t i = 0;
  const std::string date = ReadCsvField(line, &i);
  const std::string mood_str = ReadCsvField(line, &i);
  const std::string note = ReadCsvField(line, &i);

  Entry e;
  e.date = date;

  try {
    e.mood = std::stoi(mood_str);
  } catch (...) {
    throw std::runtime_error("Invalid mood in CSV: " + mood_str);
  }

  e.note = note;
  return e;
}

}  // namespace life_tracker
