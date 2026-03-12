#include "eq_mode_select.h"

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

}  // namespace

namespace tas58xx_select {

void print_eq_mode_options(Stream &stream) {
  stream.println("[CONSOLE] EQ mode options:");
  stream.println("[CONSOLE]   0: Off");
  stream.println("[CONSOLE]   1: EQ 15 Band");
  stream.println("[CONSOLE]   2: EQ BIAMP 15 Band");
  stream.println("[CONSOLE]   3: EQ Presets");
}

bool apply_eq_mode(Tas58xx &amp, const String &value_text, Stream &stream) {
  int mode = 0;
  if (!parse_int_range(value_text, 0, 3, &mode)) {
    stream.println("[CONSOLE] EQ mode out of range. Use eqmode:0..3");
    return false;
  }

  if (!amp.setEqMode(static_cast<Tas58xx::EqMode>(mode))) {
    stream.println("[CONSOLE] Failed to set EQ mode");
    return false;
  }

  stream.printf("[CONSOLE] EQ mode set to %d\n", mode);
  return true;
}

}  // namespace tas58xx_select
