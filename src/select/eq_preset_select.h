#pragma once

#include <Arduino.h>
#include "../tas58xx.h"

namespace tas58xx_select {

void print_eq_preset_options(Stream &stream);
bool apply_eq_preset_left(Tas58xx &amp, const String &value_text, Stream &stream);
bool apply_eq_preset_right(Tas58xx &amp, const String &value_text, Stream &stream);

}  // namespace tas58xx_select
