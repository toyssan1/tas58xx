#include "channel_volume_number.h"

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

bool apply_channel_volume(Tas58xx &amp, Tas58xx::Channel channel, const String &value_text, const char *name, Stream &stream) {
  int volume_db = 0;
  if (!parse_int_range(value_text, -24, 24, &volume_db)) {
    stream.printf("[CONSOLE] %s channel volume out of range. Use -24..24\n", name);
    return false;
  }

  if (!amp.setChannelVolumeDb(channel, static_cast<int8_t>(volume_db))) {
    stream.printf("[CONSOLE] Failed to set %s channel volume\n", name);
    return false;
  }

  stream.printf("[CONSOLE] %s channel volume set to %d dB\n", name, volume_db);
  return true;
}

}  // namespace

namespace tas58xx_number {

bool apply_channel_volume_left(Tas58xx &amp, const String &value_text, Stream &stream) {
  return apply_channel_volume(amp, Tas58xx::Channel::Left, value_text, "Left", stream);
}

bool apply_channel_volume_right(Tas58xx &amp, const String &value_text, Stream &stream) {
  return apply_channel_volume(amp, Tas58xx::Channel::Right, value_text, "Right", stream);
}

}  // namespace tas58xx_number
