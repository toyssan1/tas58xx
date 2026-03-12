#include "eq_preset_select.h"

namespace {

constexpr const char *kEqPresetNames[] = {
    "Flat", "LF 60Hz", "LF 70Hz", "LF 80Hz", "LF 90Hz", "LF 100Hz", "LF 110Hz", "LF 120Hz", "LF 130Hz", "LF 140Hz", "LF 150Hz",
    "HF 60Hz", "HF 70Hz", "HF 80Hz", "HF 90Hz", "HF 100Hz", "HF 110Hz", "HF 120Hz", "HF 130Hz", "HF 140Hz", "HF 150Hz",
};

constexpr int kEqPresetCount = static_cast<int>(sizeof(kEqPresetNames) / sizeof(kEqPresetNames[0]));

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

bool parse_eq_preset(const String &value_text, int *preset_index) {
  if (preset_index == nullptr) return false;

  String text = value_text;
  text.trim();
  if (text.length() == 0) return false;

  int parsed_index = 0;
  if (parse_int_range(text, 0, kEqPresetCount - 1, &parsed_index)) {
    *preset_index = parsed_index;
    return true;
  }

  for (int i = 0; i < kEqPresetCount; i++) {
    if (text.equalsIgnoreCase(kEqPresetNames[i])) {
      *preset_index = i;
      return true;
    }
  }

  String normalized = text;
  normalized.replace(" ", "");
  normalized.replace("-", "");
  normalized.toLowerCase();

  for (int i = 0; i < kEqPresetCount; i++) {
    String candidate = String(kEqPresetNames[i]);
    candidate.replace(" ", "");
    candidate.replace("-", "");
    candidate.toLowerCase();
    if (candidate == normalized) {
      *preset_index = i;
      return true;
    }
  }

  return false;
}

bool apply_eq_preset(Tas58xx &amp, Tas58xx::Channel channel, const String &value_text, const char *channel_name, Stream &stream) {
  int preset = 0;
  if (!parse_eq_preset(value_text, &preset)) {
    stream.printf("[CONSOLE] Invalid preset for %s. Use <0..20|name> or 'presets'\n", channel_name);
    return false;
  }

  if (!amp.setEqPreset(channel, static_cast<uint8_t>(preset))) {
    stream.printf("[CONSOLE] Failed to set %s preset\n", channel_name);
    return false;
  }

  stream.printf("[CONSOLE] %s preset set to %d (%s)\n", channel_name, preset, kEqPresetNames[preset]);
  return true;
}

}  // namespace

namespace tas58xx_select {

void print_eq_preset_options(Stream &stream) {
  stream.println("[CONSOLE] EQ preset options:");
  for (int i = 0; i < kEqPresetCount; i++) {
    stream.printf("[CONSOLE]   %2d: %s\n", i, kEqPresetNames[i]);
  }
}

bool apply_eq_preset_left(Tas58xx &amp, const String &value_text, Stream &stream) {
  return apply_eq_preset(amp, Tas58xx::Channel::Left, value_text, "Left", stream);
}

bool apply_eq_preset_right(Tas58xx &amp, const String &value_text, Stream &stream) {
  return apply_eq_preset(amp, Tas58xx::Channel::Right, value_text, "Right", stream);
}

}  // namespace tas58xx_select
