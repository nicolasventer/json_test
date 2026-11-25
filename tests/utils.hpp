#pragma once

#include <gtest/gtest.h>

#include <vector>

template <class Container, class... Preds>
bool VectorContainsAll(const Container& actual, Preds... preds) {
  std::vector<char> used(actual.size(), 0);
  bool ok = true;
  auto take = [&](auto pred) {
    for (size_t i = 0; i < actual.size(); ++i) {
      if (used[i] == 0 && pred(actual[i])) {
        used[i] = 1;
        return true;
      }
    }
    return false;
  };
  ((ok = ok && take(preds)), ...);
  return ok;
}

template <class Container, class... Preds>
bool VectorContainsNone(const Container& actual, Preds... preds) {
  bool found = false;
  auto matches = [&](auto pred) {
    for (const auto& item : actual) {
      if (pred(item)) {
        return true;
      }
    }
    return false;
  };
  ((found = found || matches(preds)), ...);
  return !found;
}

#define EXPECT_CONTAINS(...) EXPECT_TRUE(VectorContainsAll(__VA_ARGS__))
#define EXPECT_NOT_CONTAINS(...) EXPECT_TRUE(VectorContainsNone(__VA_ARGS__))
