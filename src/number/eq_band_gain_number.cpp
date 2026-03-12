#include "eq_band_gain_number.h"

namespace {

bool parse_int_range(const String &value_text, int min_value, int max_value, int *result) {
  if (result == nullptr || value_text.length() == 0) return false;

  int start = 0;
  if (value_text[0] == '+' || value_text[0] == '-') {
    if (value_text.length() == 1) return false;
    start = 1;
  }

  for (int i = start; i < value_text.length(); i++) {
    if (!isDigit(static_cast<unsigned char>(value_text[i]))) {
      return false;
    }
  }

  const int parsed = value_text.toInt();
  if (parsed < min_value || parsed > max_value) return false;
  *result = parsed;
  return true;
}

bool parse_band_gain(const String &value_text, int *band_index, int *gain_db) {
  if (band_index == nullptr || gain_db == nullptr) return false;
  const int split = value_text.indexOf(':');
  if (split <= 0 || split >= static_cast<int>(value_text.length() - 1)) return false;

  int parsed_band = 0;
  int parsed_gain = 0;
  if (!parse_int_range(value_text.substring(0, split), 0, 14, &parsed_band)) return false;
  if (!parse_int_range(value_text.substring(split + 1), -15, 15, &parsed_gain)) return false;

  *band_index = parsed_band;
  *gain_db = parsed_gain;
  return true;
}

bool apply_eq_band_gain(Tas58xx &amp, Tas58xx::Channel channel, const String &value_text, const char *name, Stream &stream) {
  int band_index = 0;
  int gain_db = 0;
  if (!parse_band_gain(value_text, &band_index, &gain_db)) {
    stream.printf("[CONSOLE] Invalid %s EQ format. Use <band 0..14>:<gain -15..15>\n", name);
    return false;
  }

  if (!amp.setEqBandGain(channel, static_cast<uint8_t>(band_index), static_cast<int8_t>(gain_db))) {
    stream.printf("[CONSOLE] Failed to set %s EQ band gain\n", name);
    return false;
  }

  stream.printf("[CONSOLE] %s EQ band %d set to %d dB\n", name, band_index, gain_db);
  return true;
}

}  // namespace

namespace tas58xx_number {

bool apply_eq_band_gain_left(Tas58xx &amp, const String &value_text, Stream &stream) {
  return apply_eq_band_gain(amp, Tas58xx::Channel::Left, value_text, "Left", stream);
}

bool apply_eq_band_gain_right(Tas58xx &amp, const String &value_text, Stream &stream) {
  return apply_eq_band_gain(amp, Tas58xx::Channel::Right, value_text, "Right", stream);
}

}  // namespace tas58xx_number
