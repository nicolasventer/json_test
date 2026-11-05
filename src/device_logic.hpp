#pragma once

#include "IModule.hpp"
#include "Timer.hpp"

#include <cstddef>

namespace json_test {

class DeviceLogic : public IModuleLogic {
 public:
  DeviceLogic(IModule& module, Timer& timer);
  ~DeviceLogic() override;

  DeviceLogic(const DeviceLogic&) = delete;
  DeviceLogic& operator=(const DeviceLogic&) = delete;

  void onReceive(cmd message) override;
  void onReceive(config message) override;
  void update();

 private:
  void onTick(int index, int tick_ms);

  IModule& module_;
  Timer& timer_;
  config config_{};
  bool has_config_ = false;
  bool started_ = false;
  int processed_ticks_ = 0;
  std::size_t on_sleep_id_ = 0;
};

}  // namespace json_test
