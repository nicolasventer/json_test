#include "suite_runner.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

const std::vector<std::string>& example1_names() {
  static const std::vector<std::string> names = json_test::list_test_names(JSON_TEST_SUITE_PATH);
  return names;
}

class ModuleSuite : public testing::TestWithParam<std::string> {};

TEST_P(ModuleSuite, Runs) {
  const json_test::RunResult result = json_test::run_named_test(JSON_TEST_SUITE_PATH, GetParam());
  if (result.skipped) {
    GTEST_SKIP() << result.skip_reason;
  }
  if (!result.failure.empty()) {
    FAIL() << result.failure;
  }
}

INSTANTIATE_TEST_SUITE_P(Example1, ModuleSuite, testing::ValuesIn(example1_names()));

}  // namespace
