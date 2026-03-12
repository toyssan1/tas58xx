#pragma once

#include <Arduino.h>
#include "../tas58xx.h"

namespace tas58xx_select {

void print_eq_mode_options(Stream &stream);
bool apply_eq_mode(Tas58xx &amp, const String &value_text, Stream &stream);

}  // namespace tas58xx_select
