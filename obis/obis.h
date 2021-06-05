#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
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

class OBISChannel : public sensor::Sensor {
  friend class OBISComponent;

 public:
  void set_channel(string channel) { channel_ = channel; }

 protected:
  string channel_;
};

class OBISComponent : public Component, public uart::UARTDevice {
 protected:
  std::map<std::string, sensor::Sensor *> channels_;
  void handle_line(char *line);

 public:
  void setup() override;
  void loop() override;
  void register_channel(OBISChannel *channel) { this->channels_[channel->channel_] = channel; }
};

}  // namespace obis
}  // namespace esphome
