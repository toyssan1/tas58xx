#pragma once

#include <Arduino.h>
#include <Wire.h>

class Tas58xx {
 public:
  enum class Channel : uint8_t {
    Left = 0,
    Right = 1,
  };

  enum class ControlState : uint8_t {
    DeepSleep = 0x00,
    Sleep = 0x01,
    HiZ = 0x02,
    Play = 0x03,
  };

  enum class DacMode : uint8_t {
    Btl = 0,
    Pbtl = 1,
  };

  enum class ModulationScheme : uint8_t {
    Bd = 0,
    OneSpw = 1,
  };

  enum class MixerMode : uint8_t {
    Stereo = 0,
    StereoInverse = 1,
    Mono = 2,
    Right = 3,
    Left = 4,
  };

  enum class EqMode : uint8_t {
    Off = 0,
    On15Band = 1,
    Biamp15Band = 2,
    Presets = 3,
  };

  struct FaultStatus {
    uint8_t channel_fault{0};
    uint8_t global_fault1{0};
    uint8_t global_fault2{0};
    uint8_t ot_warning{0};
  };

  explicit Tas58xx(uint8_t i2c_addr = 0x4C);

  bool begin(TwoWire &wire, int sda, int scl, uint32_t clock_hz = 400000, int pdn_pin = -1);
  bool isPresent() const;

  bool applyMinimalInit();
  bool clearFaults();
  bool readFaults(FaultStatus &faults) const;
  uint32_t timesFaultsCleared() const;

  static bool hasClockFault(const FaultStatus &faults);
  static bool hasNonClockFault(const FaultStatus &faults);
  static bool hasAnyFault(const FaultStatus &faults, bool include_clock_fault = true);

  bool setVolume(float level_0_to_1);
  bool setVolumeRaw(uint8_t raw_volume);
  bool readVolumeRaw(uint8_t &raw_volume) const;

  bool setMute(bool mute);
  bool setControlState(ControlState state);
  bool setDacMode(DacMode mode);
  bool setModulationScheme(ModulationScheme mode);
  bool setAnalogGainDb(float gain_db);

  bool setMixerMode(MixerMode mode);
  bool setChannelVolumeDb(Channel channel, int8_t volume_db);

  bool setEqMode(EqMode mode);
  bool setEqBandGain(Channel channel, uint8_t band_index, int8_t gain_db);
  bool setEqPreset(Channel channel, uint8_t preset_index);

  bool writeRegister(uint8_t reg, uint8_t value);
  bool readRegister(uint8_t reg, uint8_t &value) const;

 private:
  bool writeBytes(uint8_t reg, const uint8_t *data, size_t len);
  bool readBytes(uint8_t reg, uint8_t *data, size_t len) const;
  bool bookPageWrite(uint8_t book, uint8_t page, uint8_t sub_addr, const uint8_t *data, uint8_t bytes);

  static uint8_t volumeToRaw(float level_0_to_1);

  TwoWire *wire_{nullptr};
  uint8_t i2c_addr_{0x4C};
  uint32_t times_faults_cleared_{0};
};
