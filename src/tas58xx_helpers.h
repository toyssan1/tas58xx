#pragma once

#include <Arduino.h>

namespace tas58xx_data {

int32_t gain_to_f9_23(int8_t gain_db);

}  // namespace tas58xx_data
