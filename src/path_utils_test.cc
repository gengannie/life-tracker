#include "src/path_utils.h"

#include <cstdlib>
#include <filesystem>
#include <string>

#include "gtest/gtest.h"

namespace life_tracker {
namespace {

class EnvVarGuard {
 public:
  explicit EnvVarGuard(const char* name) : name_(name) {
    const char* existing = std::getenv(name_);
    if (existing != nullptr) {
      had_original_ = true;
      original_ = existing;
    }
  }

  EnvVarGuard(const EnvVarGuard&) = delete;
  EnvVarGuard& operator=(const EnvVarGuard&) = delete;

  ~EnvVarGuard() { Restore(); }

  void Set(const char* value) {
#ifdef _WIN32
    if (value == nullptr) {
      _putenv_s(name_, "");
    } else {
      _putenv_s(name_, value);
    }
#else
    if (value == nullptr) {
      unsetenv(name_);
    } else {
      setenv(name_, value, 1);
    }
#endif
  }

  void Unset() { Set(nullptr); }

 private:
  void Restore() {
    if (had_original_) {
      Set(original_.c_str());
    } else {
      Unset();
    }
  }

  const char* name_;
  bool had_original_ = false;
  std::string original_;
};

}  // namespace

TEST(ResolveDataPathTest, RelativeUsesWorkspaceRootWhenSet) {
  EnvVarGuard env("BUILD_WORKSPACE_DIRECTORY");
  env.Set("/tmp/ws");

  const std::filesystem::path expected = std::filesystem::path("/tmp/ws") / "data/entries.csv";
  EXPECT_EQ(ResolveDataPath("data/entries.csv"), expected.lexically_normal().string());
}

TEST(ResolveDataPathTest, RelativeFallsBackToCurrentDirectory) {
  EnvVarGuard env("BUILD_WORKSPACE_DIRECTORY");
  env.Unset();

  const std::filesystem::path expected = std::filesystem::current_path() / "data/entries.csv";
  EXPECT_EQ(ResolveDataPath("data/entries.csv"), expected.lexically_normal().string());
}

TEST(ResolveDataPathTest, AbsolutePathReturnedAsIs) {
  EnvVarGuard env("BUILD_WORKSPACE_DIRECTORY");
  env.Set("/tmp/ws");

  const std::string absolute_path = "/var/tmp/../data/entries.csv";
  EXPECT_EQ(ResolveDataPath(absolute_path),
            std::filesystem::path(absolute_path).lexically_normal().string());
}

}  // namespace life_tracker
