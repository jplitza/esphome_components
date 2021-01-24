#include "esphome.h"
#include <map>

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

#define OBIS_BUFSIZE 512

class OBISSensor : public Component, public uart::UARTDevice {
    protected:
        std::vector<std::string> fields;
        std::map<std::string, sensor::Sensor *> sensors;

    public:
        OBISSensor(uart::UARTComponent *parent, std::vector<std::string> fields) : uart::UARTDevice(parent) {
            // we need to store the actual vector so the order is preserved
            this->fields = fields;
            for (const auto &field : fields) {
                sensors[field] = new Sensor();
            }
        }

        std::vector<sensor::Sensor *> get_sensors() {
            // output in order that the fields were given to constructor
            std::vector<sensor::Sensor *> sensor_vector = {};
            for (const auto &field : this->fields) {
                sensor_vector.push_back(this->sensors[field]);
            }
            return sensor_vector;
        }

        void setup() override {
            setTimeout(100);
        }

        void loop() override {
            char buf[OBIS_BUFSIZE];
            size_t len = readBytesUntil('\n', buf, OBIS_BUFSIZE - 1);
            if (len == 0)
                return;

            buf[len] = '\0';
            if (buf[len-1] == '\r')
                buf[len-1] = '\0';

            ESP_LOGVV("OBIS", "Received: '%s'", buf);

            this->handle_line(buf);
        }

        void handle_line(char *line) {
            char *value, *unit, *trailer, *field = line;
            if (line == NULL) {
                ESP_LOGE(
                    "OBIS",
                    "handle_line() called with NULL pointer");
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
                ESP_LOGW(
                    "OBIS",
                    "Format error: Missing opening bracket: '%s'",
                    line);
                return;
            }

            unit = strchr(value, '*');

            trailer = strchr(unit != NULL? unit : value, ')');
            if (trailer == NULL) {
                ESP_LOGW(
                    "OBIS",
                    "Format error: Missing closing bracket: '%s'",
                    line);
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
                ESP_LOGW(
                    "OBIS",
                    "Format error: Trailing line: '%s'",
                    trailer);
            }

            ESP_LOGD(
                "OBIS",
                "Found field '%s' with value '%s' and unit '%s'",
                line,
                value,
                unit);

            for (const auto &sensor : this->sensors) {
                if (!sensor.first.compare(field)) {
                    if (sensor.second->get_unit_of_measurement().compare(unit)) {
                        ESP_LOGW(
                            "OBIS",
                            "Unit of measurement mismatch for field '%s': "
                            "'%s' is configured, but '%s' was sent. "
                            "Ignoring measurement.",
                            field,
                            sensor.second->get_unit_of_measurement().c_str(),
                            unit);
                        return;
                    }
                    char *remain;
                    double fvalue = strtod(value, &remain);
                    if (*remain != '\0') {
                        ESP_LOGW(
                            "OBIS",
                            "Format error: Non-numeric value: %s. "
                            "Ignoring measurement.",
                            value);
                        return;
                    }
                    sensor.second->publish_state(fvalue);
                }
            }
        }
};
