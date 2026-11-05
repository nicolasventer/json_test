#pragma once

#include <string>
#include <vector>

namespace json_test {

struct RunResult {
  bool skipped = false;
  std::string skip_reason;
  std::string failure;
};

std::vector<std::string> list_test_names(const std::string& suite_path);
RunResult run_named_test(const std::string& suite_path, const std::string& test_name);

}  // namespace json_test
