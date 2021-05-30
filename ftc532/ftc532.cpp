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

#include "ftc532.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ftc532 {

static const char *TAG = "ftc532";

void FTC532Component::dump_config() {
  ESP_LOGCONFIG(TAG, "FTC532:");
  LOG_PIN("  Pin: ", this->pin_);
  ESP_LOGCONFIG(TAG, "  Debounce: %d", this->debounce_);
}

void FTC532Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up FTC532...");
  this->state = FTC532_STATE_WAITING;
  this->pin_->setup();
  this->rxtime = micros();
  this->pin_->attach_interrupt(FTC532Component::ISR, this, RISING);
}


void FTC532Component::ISR(FTC532Component *arg) {   // Hardware interrupt routine, triggers on rising edge
  uint32_t time = micros();
  uint32_t time_diff = time - arg->rxtime;
  ESP_LOGVV(TAG, "interrupt handler fired after %d micros", time_diff);
  arg->rxtime = time;

  if (arg->state & (FTC532_STATE_WAITING | FTC532_STATE_COMPLETE)) {
    if (time_diff > FTC532_LONG + FTC532_SHORT) {   // new frame
      arg->rxbit = 0;
      if (arg->state & FTC532_STATE_COMPLETE) {
        arg->sample = arg->tsmp;      // copy completed frame
#ifdef DEBUG_FTC532
        arg->valid = true;
#endif  // DEBUG_FTC532
      }
      arg->state = FTC532_STATE_READING;
      arg->tsmp = 0;
    } else {
      arg->state = FTC532_STATE_WAITING;
    }
    return;
  }
  // FTC532_STATE_READING starts here
  if (time_diff > FTC532_LONG + FTC532_BIT) {
#ifdef DEBUG_FTC532
    ++arg->e_frame;                          // frame error
#endif  // DEBUG_FTC532
    arg->state = FTC532_STATE_WAITING;
    return;
  }
  if (time_diff > FTC532_SHORT + FTC532_BIT) {
    arg->tsmp |= (1 << arg->rxbit);   // LONG
  } else if (time_diff < FTC532_NOISE) {            // NOISE (SHORT now implicitly default)
#ifdef DEBUG_FTC532
    ++arg->e_noise;
#endif  // DEBUG_FTC532
    arg->state = FTC532_STATE_WAITING;
    return;
  }
  ++arg->rxbit;
  if (arg->rxbit == FTC532_KEYS_MAX * 2) {   // frame complete
    arg->state = FTC532_STATE_COMPLETE;
  }
}

void FTC532Component::loop() {
  if ((this->sample & 0xF0F0) == ((~this->sample & 0x0F0F) << 4) && (this->sample >> 8) == 0xF0) {
    this->keys = this->sample & 0xF;
    if (this->keys != this->old_keys) {
      if (++this->key_cnt >= this->debounce_) {
#ifdef DEBUG_FTC532
        ESP_LOGD(TAG, "SAM=%04X KEY=%X OLD=%X INV=%u NOI=%u FRM=%u OK=%u TIME=%lu Pin=%u",
                 this->sample, this->keys, this->old_keys, this->e_inv, this->e_noise, this->e_frame,
                 this->valid, this->rxtime, this->pin->get_pin());
#endif  // DEBUG_FTC532
        for (auto *channel : this->channels_)
          channel->process(this->keys);
        this->old_keys = this->keys;
        this->key_cnt = 0;
      }
    } else {
      this->key_cnt = 0;
    }
  }
#ifdef DEBUG_FTC532
  else {
    ++this->e_inv;
    ESP_LOGD(TAG, "ILL SAM=%04X", this->sample);
  }
#endif  // DEBUG_FTC532
}

}  // namespace ftc532
}  // namespace esphome
