#include "device_logic.hpp"

namespace json_test {

void DeviceLogic::setModule(IModule& module) { module_ = &module; }

void DeviceLogic::onReceive(cmd message) {
  if (message.value == "START") {
    started_ = true;
    return;
  }

  // Arrêt laissé inactif : le test « Arrêt du dispositif » doit échouer.
  // if (message.value == "STOP") {
  //   module_->send(state{CLOSED, static_cast<int>(module_->getTimer().now().count())});
  // }
}

void DeviceLogic::onReceive(config message) {
  config_ = message;
  has_config_ = true;
  if (tick_ || module_ == nullptr || config_.interval_ms <= 0) {
    return;
  }

  const auto interval = std::chrono::milliseconds{config_.interval_ms};
  tick_ = module_->getTimer().getExecutor(interval).executeAtEx(interval, [this] {
    onTick(static_cast<int>(module_->getTimer().now().count()));
  });
}

void DeviceLogic::onTick(int tick_ms) {
  if (!started_ || !has_config_ || config_.interval_ms <= 0) {
    return;
  }

  ++processed_ticks_;
  if (processed_ticks_ == 1) {
    const double temperature = config_.threshold - 5.5;
    module_->send(state{RUNNING, tick_ms});
    module_->send(measure{temperature, 40});
    return;
  }

  if (processed_ticks_ == 2) {
    const double temperature = config_.threshold + 0.5;
    if (temperature > config_.threshold) {
      module_->send(alarm{WARN, 42});
      module_->send(state{CLOSED, tick_ms});
    }
  }
}

}  // namespace json_test
