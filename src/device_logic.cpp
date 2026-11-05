#include "device_logic.hpp"

namespace json_test {

DeviceLogic::DeviceLogic(IModule& module, Timer& timer) : module_(module), timer_(timer) {
  on_sleep_id_ = timer_.addOnSleep([this] { update(); });
}

DeviceLogic::~DeviceLogic() { timer_.removeOnSleep(on_sleep_id_); }

void DeviceLogic::onReceive(cmd message) {
  if (message.value == "START") {
    started_ = true;
    return;
  }

  // Arrêt laissé inactif : le test « Arrêt du dispositif » doit échouer.
  // if (message.value == "STOP") {
  //   module_.send(state{CLOSED, static_cast<int>(timer_.now().count())});
  // }
}

void DeviceLogic::onReceive(config message) {
  config_ = message;
  has_config_ = true;
}

void DeviceLogic::update() {
  if (!started_ || !has_config_ || config_.interval_ms <= 0) {
    return;
  }

  const auto current = timer_.now();
  while (true) {
    const auto next_tick = std::chrono::milliseconds{(processed_ticks_ + 1) * config_.interval_ms};
    if (next_tick > current) {
      break;
    }

    ++processed_ticks_;
    onTick(processed_ticks_, static_cast<int>(next_tick.count()));
  }
}

void DeviceLogic::onTick(int index, int tick_ms) {
  if (index == 1) {
    const double temperature = config_.threshold - 5.5;
    module_.send(state{RUNNING, tick_ms});
    module_.send(measure{temperature, 40});
    return;
  }

  if (index == 2) {
    const double temperature = config_.threshold + 0.5;
    if (temperature > config_.threshold) {
      module_.send(alarm{WARN, 42});
      module_.send(state{CLOSED, tick_ms});
    }
  }
}

}  // namespace json_test
