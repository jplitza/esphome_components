# OBIS power meter sensor

[ESPHome] component that reads the D0 interface of an EasyMeter Q3D (and
probably others) via the hardware UART interface of an ESP8266.

[ESPHome]: https://esphome.io/

## Config

```yaml
esphome:
  includes:
    - obis_sensor.h

logger:
  baud_rate: 9600

uart:
  id: uart_bus
  rx_pin: GPIO3
  baud_rate: 9600

sensor:
  - platform: custom
    lambda: |-
      auto obis_sensor = new OBISSensor(
        id(uart_bus),
        {
            "1-0:1.8.0*255",
            "1-0:21.7.255*255",
            "1-0:41.7.255*255",
            "1-0:61.7.255*255",
            "1-0:1.7.255*255"
        });
      App.register_component(obis_sensor);
      return obis_sensor->get_sensors();

    sensors:
      - name: "Power Meter"
        unit_of_measurement: "kWh"
        accuracy_decimals: 7
      - name: "Phase 1 Power"
        unit_of_measurement: "W"
        accuracy_decimals: 2
      - name: "Phase 2 Power"
        unit_of_measurement: "W"
        accuracy_decimals: 2
      - name: "Phase 3 Power"
        unit_of_measurement: "W"
        accuracy_decimals: 2
      - name: "Total Power"
        unit_of_measurement: "W"
        accuracy_decimals: 2
```
