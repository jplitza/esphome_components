#include "obis.h"
#include "esphome/core/log.h"

namespace esphome {
namespace obis {

static const char *TAG = "obis";

void OBISComponent::setup() {
  Serial.setTimeout(100);
}

void OBISComponent::loop() {
  char buf[OBIS_BUFSIZE];
  size_t len = Serial.readBytesUntil('\n', buf, OBIS_BUFSIZE - 1);
  if (len == 0)
    return;

  buf[len] = '\0';
  if (buf[len-1] == '\r')
    buf[len-1] = '\0';

  ESP_LOGVV(TAG, "Received: '%s'", line_ptr);

  this->handle_line(buf);
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
      return;
    case '!': // abort parsing on terminating line
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
      channel.second->publish_state(fvalue);
    }
  }
}

}  // namespace obis
}  // namespace esphome
