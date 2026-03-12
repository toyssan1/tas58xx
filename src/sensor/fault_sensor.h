#pragma once

#include <Arduino.h>
#include "../tas58xx.h"

class Tas58xxFaultSensor {
 public:
  Tas58xxFaultSensor(Tas58xx &amp, Stream &stream);

  void setPollIntervalMs(uint32_t interval_ms);
  void update();

  bool printCurrent();
  bool clearAndPrint();

 private:
  static bool statusChanged(const Tas58xx::FaultStatus &left, const Tas58xx::FaultStatus &right);
  void printStatus(const Tas58xx::FaultStatus &faults);

  Tas58xx &amp_;
  Stream &stream_;
  Tas58xx::FaultStatus last_fault_status_{};
  bool last_fault_status_valid_{false};
  bool last_has_any_fault_{false};
  uint32_t last_fault_poll_ms_{0};
  uint32_t poll_interval_ms_{1000};
};
