#include "tas58xx_helpers.h"

#include <algorithm>
#include <cmath>

namespace tas58xx_data {

int32_t gain_to_f9_23(int8_t gain_db) {
  constexpr float kLinearGainMax = 255.999999f;
  constexpr float kLinearGainMin = -256.0f;

  float linear = std::pow(10.0f, static_cast<float>(gain_db) / 20.0f);
  linear = std::max(kLinearGainMin, std::min(kLinearGainMax, linear));

  const int32_t fixed = static_cast<int32_t>(linear * (1 << 23));
  return static_cast<int32_t>(__builtin_bswap32(static_cast<uint32_t>(fixed)));
}

}  // namespace tas58xx_data
