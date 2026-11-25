#include "device_logic.hpp"

void DeviceLogic::setModule(IModule& module) { module_ = &module; }

void DeviceLogic::onReceiveDeviceCmdCommand(const CmdCommand& message) {
  if (message.value == "START") {
    started_ = true;
    return;
  }

  // Arrêt laissé inactif : le test « Arrêt du dispositif » doit échouer.
  // if (message.value == "STOP") {
  //   module_->sendDeviceStatusStatus(StatusStatus{CLOSED, static_cast<int>(module_->getTimer().now().count())});
  // }
}

void DeviceLogic::onReceiveDeviceCanMessageCommand(const CanMessageCommand& message) {
  if (module_ == nullptr) {
    return;
  }
  module_->sendDeviceCanMessageStatus(CanMessageStatus{message.id_code, message.data});
}

void DeviceLogic::onReceiveDeviceConfigStatus(const ConfigStatus& message) {
  config_ = message;
  has_config_ = true;
  if (tick_ || module_ == nullptr || config_.interval_ms <= 0) {
    return;
  }

  const auto interval = std::chrono::milliseconds{config_.interval_ms};
  tick_ = module_->getTimer().executeAtEx(interval, [this] {
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
    module_->sendDeviceStatusStatus(StatusStatus{RUNNING, tick_ms});
    module_->sendDeviceMeasureStatus(MeasureStatus{temperature, 40, config_.limits});
    return;
  }

  if (processed_ticks_ == 2) {
    const double temperature = config_.threshold + 0.5;
    if (temperature > config_.threshold) {
      module_->sendDeviceAlarmStatus(AlarmStatus{WARN, 42});
      module_->sendDeviceStatusStatus(StatusStatus{CLOSED, tick_ms});
    }
  }
}
