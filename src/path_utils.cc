#include "src/path_utils.h"

#include <cstdlib>
#include <filesystem>
#include <string>

namespace life_tracker {
namespace fs = std::filesystem;

std::string ResolveDataPath(const std::string& flag_value) {
  fs::path path(flag_value);
  if (path.is_absolute()) return path.lexically_normal().string();

  const char* workspace = std::getenv("BUILD_WORKSPACE_DIRECTORY");
  if (workspace != nullptr && workspace[0] != '\0') {
    return (fs::path(workspace) / path).lexically_normal().string();
  }

  return (fs::current_path() / path).lexically_normal().string();
}

}  // namespace life_tracker
