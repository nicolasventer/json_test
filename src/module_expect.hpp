#pragma once

#include "jwrap/jwrap.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace json_test {

class IModuleExpect {
 public:
  virtual ~IModuleExpect() = default;

  virtual void resetExpect() = 0;

  struct Sent {
    std::string topic;
    jw::Value data;
  };

  virtual const std::vector<Sent>& sent() const = 0;
  virtual std::size_t resetIndex() const = 0;
};

}  // namespace json_test
