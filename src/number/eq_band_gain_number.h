#pragma once

#include <Arduino.h>
#include "../tas58xx.h"

namespace tas58xx_number {

bool apply_eq_band_gain_left(Tas58xx &amp, const String &value_text, Stream &stream);
bool apply_eq_band_gain_right(Tas58xx &amp, const String &value_text, Stream &stream);

}  // namespace tas58xx_number
