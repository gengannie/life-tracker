#ifndef LIFE_TRACKER_PATH_UTILS_H_
#define LIFE_TRACKER_PATH_UTILS_H_

#include <string>

namespace life_tracker {

// Resolves a data path following Bazel-aware rules.
// - Absolute paths are returned as-is (normalized).
// - Relative paths are resolved against BUILD_WORKSPACE_DIRECTORY when set,
//   otherwise against the current working directory.
std::string ResolveDataPath(const std::string& flag_value);

}  // namespace life_tracker

#endif  // LIFE_TRACKER_PATH_UTILS_H_
