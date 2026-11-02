#include "json_test/json_test.hpp"

#include <gtest/gtest.h>

TEST(AddTest, AddsTwoNumbers) {
  EXPECT_EQ(json_test::add(1, 2), 3);
}
