#include "fault_sensor.h"

Tas58xxFaultSensor::Tas58xxFaultSensor(Tas58xx &amp, Stream &stream) : amp_(amp), stream_(stream) {}

void Tas58xxFaultSensor::setPollIntervalMs(uint32_t interval_ms) {
  poll_interval_ms_ = interval_ms;
}

bool Tas58xxFaultSensor::statusChanged(const Tas58xx::FaultStatus &left, const Tas58xx::FaultStatus &right) {
  return left.channel_fault != right.channel_fault || left.global_fault1 != right.global_fault1 || left.global_fault2 != right.global_fault2 ||
         left.ot_warning != right.ot_warning;
}

void Tas58xxFaultSensor::printStatus(const Tas58xx::FaultStatus &faults) {
  stream_.printf("[FAULT] CH=0x%02X G1=0x%02X G2=0x%02X OT=0x%02X\n", faults.channel_fault, faults.global_fault1, faults.global_fault2, faults.ot_warning);
  stream_.printf("[FAULT] has_non_clock=%s has_clock=%s has_any=%s clears=%lu\n", Tas58xx::hasNonClockFault(faults) ? "yes" : "no",
                 Tas58xx::hasClockFault(faults) ? "yes" : "no", Tas58xx::hasAnyFault(faults, true) ? "yes" : "no",
                 static_cast<unsigned long>(amp_.timesFaultsCleared()));
}

void Tas58xxFaultSensor::update() {
  const uint32_t now = millis();
  if ((now - last_fault_poll_ms_) < poll_interval_ms_) {
    return;
  }
  last_fault_poll_ms_ = now;

  Tas58xx::FaultStatus faults{};
  if (!amp_.readFaults(faults)) {
    return;
  }

  const bool has_any_fault = Tas58xx::hasAnyFault(faults, true);
  const bool changed = !last_fault_status_valid_ || statusChanged(faults, last_fault_status_) || (has_any_fault != last_has_any_fault_);
  if (changed) {
    stream_.printf("[FAULT] State changed: %s\n", has_any_fault ? "FAULT" : "CLEAR");
    printStatus(faults);
    last_fault_status_ = faults;
    last_fault_status_valid_ = true;
    last_has_any_fault_ = has_any_fault;
  }
}

bool Tas58xxFaultSensor::printCurrent() {
  Tas58xx::FaultStatus faults{};
  if (!amp_.readFaults(faults)) {
    return false;
  }

  printStatus(faults);
  last_fault_status_ = faults;
  last_fault_status_valid_ = true;
  last_has_any_fault_ = Tas58xx::hasAnyFault(faults, true);
  return true;
}

bool Tas58xxFaultSensor::clearAndPrint() {
  if (!amp_.clearFaults()) {
    return false;
  }

  stream_.printf("[CONSOLE] Faults cleared, count=%lu\n", static_cast<unsigned long>(amp_.timesFaultsCleared()));
  return this->printCurrent();
}
