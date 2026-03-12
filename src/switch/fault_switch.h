#pragma once

#include <Arduino.h>
#include "../tas58xx.h"

namespace tas58xx_switch {

bool clear_faults(Tas58xx &amp, Stream &stream);

}  // namespace tas58xx_switch
