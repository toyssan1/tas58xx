#include "tas58xx.h"

#include <algorithm>
#include <cmath>

#define USE_TAS58XX_EQ_GAINS
#define USE_TAS58XX_EQ_PRESETS
#include "tas58xx_defs.h"
#include "tas58xx_eq_common.h"
#include "tas58xx_eq_bands.h"
#include "tas58xx_eq_profiles.h"
#include "tas58xx_helpers.h"
#include "tas58xx_minimal.h"

namespace {

using namespace esphome::tas58xx;
using namespace tas58xx_data;

constexpr int8_t kEqMinDb = -15;
constexpr int8_t kEqMaxDb = 15;

}  // namespace

Tas58xx::Tas58xx(uint8_t i2c_addr) : i2c_addr_(i2c_addr) {}

bool Tas58xx::begin(TwoWire &wire, int sda, int scl, uint32_t clock_hz, int pdn_pin) {
  wire_ = &wire;

  if (pdn_pin >= 0) {
    pinMode(static_cast<uint8_t>(pdn_pin), OUTPUT);
    digitalWrite(static_cast<uint8_t>(pdn_pin), LOW);
    delay(2);
    digitalWrite(static_cast<uint8_t>(pdn_pin), HIGH);
    delay(10);
  }

  wire_->begin(sda, scl);
  wire_->setClock(clock_hz);

  return this->isPresent();
}

bool Tas58xx::isPresent() const {
  if (wire_ == nullptr) return false;
  wire_->beginTransmission(i2c_addr_);
  return wire_->endTransmission() == 0;
}

bool Tas58xx::applyMinimalInit() {
  for (const auto &entry : TAS58XX_CONFIG) {
    if (entry.addr == TAS58XX_CFG_META_DELAY) {
      delay(entry.value);
      continue;
    }
    if (!this->writeRegister(entry.addr, entry.value)) {
      return false;
    }
  }
  return true;
}

bool Tas58xx::clearFaults() {
  return this->writeRegister(TAS58XX_FAULT_CLEAR, TAS58XX_ANALOG_FAULT_CLEAR);
}

bool Tas58xx::readFaults(FaultStatus &faults) const {
  uint8_t data[4] = {0};
  if (!this->readBytes(TAS58XX_CHAN_FAULT, data, sizeof(data))) {
    return false;
  }

  faults.channel_fault = data[0];
  faults.global_fault1 = data[1];
  faults.global_fault2 = data[2];
  faults.ot_warning = data[3];
  return true;
}

bool Tas58xx::setVolume(float level_0_to_1) {
  return this->setVolumeRaw(volumeToRaw(level_0_to_1));
}

bool Tas58xx::setVolumeRaw(uint8_t raw_volume) {
  return this->writeRegister(TAS58XX_DIG_VOL_CTRL, raw_volume);
}

bool Tas58xx::readVolumeRaw(uint8_t &raw_volume) const {
  return this->readRegister(TAS58XX_DIG_VOL_CTRL, raw_volume);
}

bool Tas58xx::setMute(bool mute) {
  uint8_t ctrl2 = 0;
  if (!this->readRegister(TAS58XX_DEVICE_CTRL_2, ctrl2)) {
    return false;
  }

  constexpr uint8_t kMuteBit = 0x08;
  if (mute) {
    ctrl2 |= kMuteBit;
  } else {
    ctrl2 &= static_cast<uint8_t>(~kMuteBit);
  }
  return this->writeRegister(TAS58XX_DEVICE_CTRL_2, ctrl2);
}

bool Tas58xx::setControlState(ControlState state) {
  uint8_t ctrl2 = 0;
  if (!this->readRegister(TAS58XX_DEVICE_CTRL_2, ctrl2)) {
    return false;
  }

  const uint8_t mute_bit = ctrl2 & 0x08;
  const uint8_t state_bits = static_cast<uint8_t>(state) & 0x03;
  return this->writeRegister(TAS58XX_DEVICE_CTRL_2, mute_bit | state_bits);
}

bool Tas58xx::setDacMode(DacMode mode) {
  uint8_t ctrl1 = 0;
  if (!this->readRegister(TAS58XX_DEVICE_CTRL_1, ctrl1)) {
    return false;
  }

  constexpr uint8_t kPbtlBit = 1 << 2;
  if (mode == DacMode::Pbtl) {
    ctrl1 |= kPbtlBit;
  } else {
    ctrl1 &= static_cast<uint8_t>(~kPbtlBit);
  }
  return this->writeRegister(TAS58XX_DEVICE_CTRL_1, ctrl1);
}

bool Tas58xx::setModulationScheme(ModulationScheme mode) {
  uint8_t ctrl1 = 0;
  if (!this->readRegister(TAS58XX_DEVICE_CTRL_1, ctrl1)) {
    return false;
  }

  ctrl1 &= 0xFC;
  ctrl1 |= static_cast<uint8_t>(mode);
  return this->writeRegister(TAS58XX_DEVICE_CTRL_1, ctrl1);
}

bool Tas58xx::setAnalogGainDb(float gain_db) {
  if (gain_db < -15.5f || gain_db > 0.0f) {
    return false;
  }

  uint8_t again = 0;
  if (!this->readRegister(TAS58XX_AGAIN, again)) {
    return false;
  }

  const uint8_t reserved_bits = again & 0xE0;
  const uint8_t gain_step = static_cast<uint8_t>(-gain_db * 2.0f);
  return this->writeRegister(TAS58XX_AGAIN, static_cast<uint8_t>(reserved_bits | gain_step));
}

bool Tas58xx::setMixerMode(MixerMode mode) {
  struct MixerCoefficients {
    uint32_t l_to_l;
    uint32_t r_to_l;
    uint32_t l_to_r;
    uint32_t r_to_r;
  } __attribute__((packed));

  MixerCoefficients coefficients{};

  switch (mode) {
    case MixerMode::Stereo:
      coefficients = {TAS58XX_MIXER_COEFF_0DB, TAS58XX_MIXER_COEFF_MUTE, TAS58XX_MIXER_COEFF_MUTE, TAS58XX_MIXER_COEFF_0DB};
      break;
    case MixerMode::StereoInverse:
      coefficients = {TAS58XX_MIXER_COEFF_MUTE, TAS58XX_MIXER_COEFF_0DB, TAS58XX_MIXER_COEFF_0DB, TAS58XX_MIXER_COEFF_MUTE};
      break;
    case MixerMode::Mono:
      coefficients = {TAS58XX_MIXER_COEFF_MINUS6DB, TAS58XX_MIXER_COEFF_MINUS6DB, TAS58XX_MIXER_COEFF_MINUS6DB, TAS58XX_MIXER_COEFF_MINUS6DB};
      break;
    case MixerMode::Left:
      coefficients = {TAS58XX_MIXER_COEFF_0DB, TAS58XX_MIXER_COEFF_MUTE, TAS58XX_MIXER_COEFF_0DB, TAS58XX_MIXER_COEFF_MUTE};
      break;
    case MixerMode::Right:
      coefficients = {TAS58XX_MIXER_COEFF_MUTE, TAS58XX_MIXER_COEFF_0DB, TAS58XX_MIXER_COEFF_MUTE, TAS58XX_MIXER_COEFF_0DB};
      break;
    default:
      return false;
  }

  return this->bookPageWrite(TAS58XX_AUDIO_CTRL_BOOK, TAS58XX_MIXER_GAIN_PAGE, TAS58XX_MIXER_GAIN_SUBADDR,
                             reinterpret_cast<const uint8_t *>(&coefficients), sizeof(coefficients));
}

bool Tas58xx::setChannelVolumeDb(Channel channel, int8_t volume_db) {
  if (volume_db < TAS58XX_CHANNEL_VOLUME_MIN_DB || volume_db > TAS58XX_CHANNEL_VOLUME_MAX_DB) {
    return false;
  }

  const uint8_t ch = static_cast<uint8_t>(channel);
  if (ch >= NUMBER_CHANNELS) {
    return false;
  }

  const int32_t coeff = gain_to_f9_23(volume_db);
  return this->bookPageWrite(TAS58XX_AUDIO_CTRL_BOOK, TAS58XX_CHANNEL_VOLUME_PAGE, TAS58XX_CHANNEL_VOLUME_SUBADDR[ch],
                             reinterpret_cast<const uint8_t *>(&coeff), sizeof(coeff));
}

bool Tas58xx::setEqMode(EqMode mode) {
  const uint8_t mode_index = static_cast<uint8_t>(mode);
  if (mode_index >= NUMBER_EQ_MODES) {
    return false;
  }

  const EqModeCoefficients *coeff = &TAS5825M_CTRL_EQ[mode_index];
  return this->bookPageWrite(TAS58XX_AUDIO_CTRL_BOOK, TAS5825M_EQ_MODE_CTRL_PAGE, TAS5825M_GANG_EQ,
                             reinterpret_cast<const uint8_t *>(coeff), sizeof(EqModeCoefficients));
}

bool Tas58xx::setEqBandGain(Channel channel, uint8_t band_index, int8_t gain_db) {
  if (band_index >= NUMBER_EQ_BANDS) {
    return false;
  }
  if (gain_db < kEqMinDb || gain_db > kEqMaxDb) {
    return false;
  }

  const uint8_t gain_index = static_cast<uint8_t>(gain_db + kEqMaxDb);
  const AddressSequence *eq_address = (channel == Channel::Left) ? &TAS5825M_LEFT_EQ_ADDRESS[band_index]
                                                                  : &TAS5825M_RIGHT_EQ_ADDRESS[band_index];
  const BiquadSequence *biquad = &EQ_BAND_COEFFICIENTS[gain_index][band_index];

  return this->bookPageWrite(TAS58XX_EQ_CTRL_BOOK, eq_address->page, eq_address->sub_addr,
                             biquad->coefficients, BIQUAD_SIZE);
}

bool Tas58xx::setEqPreset(Channel channel, uint8_t preset_index) {
  if (preset_index > EQ_PROFILE_MAXIMUM_INDEX) {
    return false;
  }

  const AddressSequence *addr1 = (channel == Channel::Left) ? &TAS5825M_LEFT_EQ_ADDRESS[0] : &TAS5825M_RIGHT_EQ_ADDRESS[0];
  const AddressSequence *addr2 = (channel == Channel::Left) ? &TAS5825M_LEFT_EQ_ADDRESS[1] : &TAS5825M_RIGHT_EQ_ADDRESS[1];
  const AddressSequence *addr3 = (channel == Channel::Left) ? &TAS5825M_LEFT_EQ_ADDRESS[2] : &TAS5825M_RIGHT_EQ_ADDRESS[2];

  const BiquadSequence *biquad1 = (channel == Channel::Left) ? &EQ_PROFILE_LEFT_COEFFICIENTS[preset_index][0]
                                                              : &EQ_PROFILE_RIGHT_COEFFICIENTS[preset_index][0];
  const BiquadSequence *biquad2 = (channel == Channel::Left) ? &EQ_PROFILE_LEFT_COEFFICIENTS[preset_index][1]
                                                              : &EQ_PROFILE_RIGHT_COEFFICIENTS[preset_index][1];
  const BiquadSequence *biquad3 = (channel == Channel::Left) ? &EQ_PROFILE_LEFT_COEFFICIENTS[preset_index][2]
                                                              : &EQ_PROFILE_RIGHT_COEFFICIENTS[preset_index][2];

  if (!this->bookPageWrite(TAS58XX_EQ_CTRL_BOOK, addr1->page, addr1->sub_addr, biquad1->coefficients, BIQUAD_SIZE)) return false;
  if (!this->bookPageWrite(TAS58XX_EQ_CTRL_BOOK, addr2->page, addr2->sub_addr, biquad2->coefficients, BIQUAD_SIZE)) return false;
  if (!this->bookPageWrite(TAS58XX_EQ_CTRL_BOOK, addr3->page, addr3->sub_addr, biquad3->coefficients, BIQUAD_SIZE)) return false;

  return true;
}

bool Tas58xx::writeRegister(uint8_t reg, uint8_t value) {
  return this->writeBytes(reg, &value, 1);
}

bool Tas58xx::readRegister(uint8_t reg, uint8_t &value) const {
  return this->readBytes(reg, &value, 1);
}

bool Tas58xx::writeBytes(uint8_t reg, const uint8_t *data, size_t len) {
  if (wire_ == nullptr || data == nullptr || len == 0) return false;

  wire_->beginTransmission(i2c_addr_);
  wire_->write(reg);
  for (size_t index = 0; index < len; index++) {
    wire_->write(data[index]);
  }

  return wire_->endTransmission() == 0;
}

bool Tas58xx::readBytes(uint8_t reg, uint8_t *data, size_t len) const {
  if (wire_ == nullptr || data == nullptr || len == 0) return false;

  wire_->beginTransmission(i2c_addr_);
  wire_->write(reg);
  if (wire_->endTransmission(false) != 0) {
    return false;
  }

  const size_t received = wire_->requestFrom(static_cast<int>(i2c_addr_), static_cast<int>(len));
  if (received != len) {
    return false;
  }

  for (size_t index = 0; index < len; index++) {
    if (!wire_->available()) {
      return false;
    }
    data[index] = wire_->read();
  }
  return true;
}

bool Tas58xx::bookPageWrite(uint8_t book, uint8_t page, uint8_t sub_addr, const uint8_t *data, uint8_t bytes) {
  constexpr uint8_t kPageSize = 0x80;
  constexpr uint8_t kMinPageSubAddr = 0x08;

  if (bytes == 0 || bytes > BIQUAD_SIZE) {
    return false;
  }

  uint8_t bytes_block1 = bytes;
  uint8_t bytes_block2 = 0;

  if ((sub_addr + bytes) > kPageSize) {
    bytes_block1 = kPageSize - sub_addr;
    bytes_block2 = bytes - bytes_block1;
  }

  bool ok = this->writeRegister(TAS58XX_PAGE_SET, TAS58XX_PAGE_ZERO);
  if (ok) ok = this->writeRegister(TAS58XX_BOOK_SET, book);
  if (ok) ok = this->writeRegister(TAS58XX_PAGE_SET, page);
  if (!ok) return false;

  if (!this->writeBytes(sub_addr, data, bytes_block1)) return false;

  if (bytes_block2 != 0) {
    const uint8_t next_page = static_cast<uint8_t>(page + 1);
    if (!this->writeRegister(TAS58XX_PAGE_SET, next_page)) return false;
    if (!this->writeBytes(kMinPageSubAddr, data + bytes_block1, bytes_block2)) return false;
  }

  ok = this->writeRegister(TAS58XX_PAGE_SET, TAS58XX_PAGE_ZERO);
  if (ok) ok = this->writeRegister(TAS58XX_BOOK_SET, TAS58XX_BOOK_ZERO);
  if (ok) ok = this->writeRegister(TAS58XX_PAGE_SET, TAS58XX_PAGE_ZERO);

  return ok;
}

uint8_t Tas58xx::volumeToRaw(float level_0_to_1) {
  const float clamped = std::max(0.0f, std::min(1.0f, level_0_to_1));
  const float raw_f = 254.0f - (254.0f * clamped);
  return static_cast<uint8_t>(raw_f + 0.5f);
}
