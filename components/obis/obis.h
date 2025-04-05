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

#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include <map>
#include <string>

/* Protocol (stripped to what we are interested in):
/ESY5Q3DA1004 V3.02

1-0:1.8.0*255(00024234.1010820*kWh)
1-0:21.7.255*255(000071.96*W)
1-0:41.7.255*255(000031.48*W)
1-0:61.7.255*255(000001.16*W)
1-0:1.7.255*255(000104.60*W)
1-0:96.5.5*255(82)
!

See [1, page 20] for documentation of fields.

[1] https://www.easymeter.com/downloads/products/zaehler/Q3D/Easymeter_Q3D_DE_2016-06-15.pdf
*/

using namespace std;

namespace esphome {
namespace obis {

#define OBIS_BUFSIZE 512

class OBISTrigger : public Trigger<> {};

class OBISChannelBase {
  friend class OBISComponent;

 public:
  void set_channel(string channel) { channel_ = channel; }
  virtual void publish(const char *value) = 0;
  virtual string get_unit_of_measurement() = 0;

 protected:
  string channel_;
};

class OBISChannel : public sensor::Sensor, public OBISChannelBase {
  void publish(const char *value) override;

  string get_unit_of_measurement() override {
    return sensor::Sensor::get_unit_of_measurement();
  }
};

class OBISTextChannel : public text_sensor::TextSensor, public OBISChannelBase {
 public:
  string get_unit_of_measurement() override { return ""; }
  void publish(const char *value) override;
};

class OBISComponent : public Component, public uart::UARTDevice {
 protected:
  std::map<std::string, OBISChannelBase *> channels_;
  std::vector<OBISTrigger*> opening_triggers_;
  std::vector<OBISTrigger*> closing_triggers_;

  void handle_line(char *line);
  char buf[OBIS_BUFSIZE];
  size_t index{0};

 public:
  void loop() override;
  void register_channel(OBISChannelBase *channel) { this->channels_[channel->channel_] = channel; }
  void register_opening_trigger(OBISTrigger* trigger) { opening_triggers_.push_back(trigger); }
  void register_closing_trigger(OBISTrigger* trigger) { closing_triggers_.push_back(trigger); }
};

}  // namespace obis
}  // namespace esphome
