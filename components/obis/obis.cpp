/*
  obis.cpp - Read IEC 62056-21 via serial D0 interface in ESPhome

  Copyright (C) 2019-2021  Jan-Philipp Litza

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

#include "obis.h"
#include "esphome/core/log.h"

namespace esphome {
namespace obis {

static const char *TAG = "obis";

void OBISChannel::publish(const char *value) {
  char *remain;
  double fvalue = strtod(value, &remain);
  if (*remain != '\0') {
    ESP_LOGW(
      TAG,
      "Format error: Non-numeric value: %s. "
      "Ignoring measurement.",
      value);
    return;
  }
  publish_state(fvalue);
}

void OBISTextChannel::publish(const char *value) {
  publish_state(value);
}

void OBISComponent::loop() {
  while (this->available()) {
    uint8_t byte;
    this->read_byte(&byte);

    if (this->index >= OBIS_BUFSIZE || byte == '/') {
      this->index = 0;
    }
    this->buf[this->index] = (char) byte;
    this->index++;

    if (byte == '\n') {
      this->buf[this->index - 1] = '\0';

      if (this->buf[this->index - 2] == '\r')
        this->buf[this->index - 2] = '\0';

      ESP_LOGVV(TAG, "Received: '%s'", this->buf);
      this->handle_line(this->buf);

      this->index = 0;
      break;
    }
  }  // available
}

void OBISComponent::handle_line(char *line) {
  char *value, *unit, *trailer, *field = line;
  if (line == NULL) {
    ESP_LOGE(TAG, "handle_line() called with NULL pointer");
    return;
  }

  switch(line[0]) {
    case '\0': // ignore empty lines
    case '/':  // ignore introduction line
    case '!': // ignore terminating line
      return;
  }

  value = strchr(line, '(');
  if (value == NULL) {
    ESP_LOGW(TAG, "Format error: Missing opening bracket: '%s'", line);
    return;
  }

  unit = strchr(value, '*');

  trailer = strchr(unit != NULL? unit : value, ')');
  if (trailer == NULL) {
    ESP_LOGW(TAG, "Format error: Missing closing bracket: '%s'", line);
    return;
  }

  *(value++) = '\0';
  if (unit == NULL) {
    unit = trailer;
  } else {
    *(unit++) = '\0';
  }
  *(trailer++) = '\0';

  if (*trailer != '\0') {
    ESP_LOGW(TAG, "Format error: Trailing line: '%s'", trailer);
  }

  ESP_LOGD(TAG, "Found field '%s' with value '%s' and unit '%s'", line, value, unit);

  for (const auto &channel : this->channels_) {
    if (!channel.first.compare(field)) {
      if (channel.second->get_unit_of_measurement().compare(unit)) {
        ESP_LOGW(
          TAG,
          "Unit of measurement mismatch for field '%s': "
          "'%s' is configured, but '%s' was sent. "
          "Ignoring measurement.",
          field,
          channel.second->get_unit_of_measurement().c_str(),
          unit);
        return;
      }
      channel.second->publish(value);
    }
  }
}

}  // namespace obis
}  // namespace esphome
