#include "fault_switch.h"

namespace tas58xx_switch {

bool clear_faults(Tas58xx &amp, Stream &stream) {
  if (!amp.clearFaults()) {
    stream.println("[CONSOLE] Failed to clear faults");
    return false;
  }

  stream.printf("[CONSOLE] Faults cleared, count=%lu\n", static_cast<unsigned long>(amp.timesFaultsCleared()));
  return true;
}

}  // namespace tas58xx_switch
