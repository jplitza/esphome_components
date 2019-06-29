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

#define STR(x) #x
#define XSTR(x) STR(x)
#define OBIS_BUFSIZE 512
#define OBIS_FIELDLEN 23
#define OBIS_VALUELEN 63
#define OBIS_SCANF "%" XSTR(OBIS_FIELDLEN) "[0-9A-Z:.*-](%" XSTR(OBIS_VALUELEN) "[^)])%n"

static inline bool parity(byte v) {
    // source: http://www.graphics.stanford.edu/~seander/bithacks.html#ParityParallel
    v ^= v >> 4;
    v &= 0xf;
    return (0x6996 >> v) & 1;
}

class OBISSensor : public Component, public uart::UARTDevice, public Sensor {
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

        auto get_sensors() {
            // output in order that the fields were given to constructor
            std::vector<sensor::Sensor *> sensor_vector = {};
            for (const auto &field : this->fields) {
                sensor_vector.push_back(this->sensors[field]);
            }
            return sensor_vector;
        }

        void setup() override {
            Serial.setTimeout(100);
        }

        void loop() override {
            char buf[OBIS_BUFSIZE];
            size_t len = Serial.readBytes(buf, OBIS_BUFSIZE - 1);
            if (len == 0)
                return;

            buf[len] = '\0';

            bool parity_error = false;
            char *line_ptr = buf;
            for (char *parity_ptr = buf; parity_ptr - buf < len; ++parity_ptr) {
                if (parity(*parity_ptr)) {
                    if (!parity_error) {
                        ESP_LOGD(
                            "OBIS",
                            "Parity error at character %d, ignoring until next newline",
                            parity_ptr - buf);
                        parity_error = true;
                    }
                    continue;
                }

                // mask out parity bit
                *parity_ptr &= 0x7f;

                if (*parity_ptr == '\n' || *parity_ptr == '\r') {
                    if (!parity_error) {
                        ESP_LOGVV("OBIS", "Received: '%s'", line_ptr);
                        *parity_ptr = '\0';
                        if (!this->handle_line(line_ptr))
                            break;
                    }
                    parity_error = false;
                    line_ptr = parity_ptr + 1;
                }
            }
        }

        bool handle_line(char *line) {
            if (line == NULL) {
                ESP_LOGW(
                    "OBIS",
                    "handle_line() called with NULL pointer");
                return false;
            }

            switch(line[0]) {
                case '\0': // ignore empty lines
                case '/':  // ignore introduction line
                    return true;
                case '!': // abort parsing on terminating line
                    return false;
            }

            char field[OBIS_FIELDLEN + 1];
            char value[OBIS_VALUELEN + 1];
            int matched_len;
            if (sscanf(line, OBIS_SCANF, field, value, &matched_len) == 2) {
                ESP_LOGD(
                    "OBIS",
                    "Found field '%s' with value '%s'",
                    field,
                    value);

                for (const auto &sensor : this->sensors) {
                    if (!sensor.first.compare(field)) {
                        sensor.second->publish_state(strtod(value, NULL));
                    }
                }
                if (*(line + matched_len) != '\0') {
                    ESP_LOGW(
                        "OBIS",
                        "Trailing line: '%s'",
                        line + matched_len);
                }
            } else {
                ESP_LOGW(
                    "OBIS",
                    "Unknown line: '%s'",
                    line);
            }
            return true;
        }
};
