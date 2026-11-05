#pragma once

#include "IModule.hpp"
#include "Timer.hpp"
#include "jwrap/jwrap.hpp"
#include "module_expect.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace json_test {

class TestModule : public IModule, public IModuleExpect {
 public:
  explicit TestModule(Timer& timer);

  void setLogic(IModuleLogic* logic) override;
  void Start() override;

  void send(state message) override;
  void send(measure message) override;
  void send(alarm message) override;

  void resetExpect() override;
  const std::vector<Sent>& sent() const override;
  std::size_t resetIndex() const override;

 private:
  void record(const std::string& topic, jw::Value data);

  Timer& timer_;
  IModuleLogic* logic_ = nullptr;
  std::vector<Sent> sent_;
  std::size_t reset_index_ = 0;
};

}  // namespace json_test
