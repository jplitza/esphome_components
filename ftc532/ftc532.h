/*
  xdrv_47_ftc532.ino - FTC532 touch buttons support for ESPHome
  (originally for Tasmota)

  Copyright (C) 2021  Jan-Philipp Litza (for ESPHome adaptation)
  Copyright (C) 2021  Peter Franck and Theo Arends

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "esphome/core/component.h"
#include "esphome/core/esphal.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace ftc532 {


enum {
  FTC532_STATE_WAITING  = 0x1,
  FTC532_STATE_READING  = 0x2,
  FTC532_STATE_COMPLETE = 0x4,

  FTC532_KEYS_MAX = 8,

  // Rising edge timing in microseconds
  FTC532_BIT   = 377,
  FTC532_NOISE = (FTC532_BIT * 3 / 2),
  FTC532_SHORT = (FTC532_BIT * 2),
  FTC532_LONG  = (FTC532_BIT * 4),
  FTC532_IDLE  = (FTC532_BIT * 10),
  FTC532_MAX   = (FTC532_BIT * 58),
};

class FTC532Channel : public binary_sensor::BinarySensor {
  friend class FTC532Component;

 public:
  void set_channel(uint8_t channel) { channel_ = channel; }
  void process(uint8_t data) { this->publish_state(data & (1 << this->channel_)); }

 protected:
  uint8_t channel_{0};
};

class FTC532Component : public Component {
 public:
  void register_channel(FTC532Channel *channel) { this->channels_.push_back(channel); }
  void set_pin(GPIOPin *pin) { pin_ = pin; }
  void set_debounce(uint8_t debounce) { debounce_ = debounce; }
  void setup() override;
  void dump_config() override;
  void loop() override;

 protected:
  GPIOPin *pin_;
  std::vector<FTC532Channel *> channels_{};
  uint8_t debounce_{0};

 private:
  static void ISR(FTC532Component *arg); // Hardware interrupt routine, triggers on rising edge
  volatile uint32_t rxtime;             // ISR timer memory
  volatile uint16_t tsmp    = 0;        // buffer for bit-coded time samples
  volatile uint16_t sample  = 0xF0F0;   // valid samples
  volatile uint16_t rxbit;              // ISR bit counter
  volatile uint16_t state;              // ISR state
  uint8_t keys              = 0;        // bitmap of active keys
  uint8_t old_keys          = 0;        // previously active keys
  uint8_t key_cnt           = 0;        // used to de-bounce
#ifdef DEBUG_FTC532
  volatile uint16_t e_inv   = 0;        // inverted key error counter
  volatile uint16_t e_frame = 0;        // frame error counter
  volatile uint16_t e_noise = 0;        // noise detection counter
  volatile bool valid       = 0;        // did we ever receive valid data?
#endif  // DEBUG_FTC532
};

}  // namespace ftc532
}  // namespace esphome
