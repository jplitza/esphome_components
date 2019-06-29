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

#define OBIS_SCANF "%23[0-9A-Z:.*-](%63[^)])%*[\r\n]%n"
#define OBIS_BUFSIZE 512

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
            this->fields = fields;
            for (const auto &field : fields) {
                sensors[field] = new Sensor();
            }
        }

        auto get_sensors() {
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

            for (size_t i = 0; i < len; i++) {
                if (parity(buf[i])) {
                    ESP_LOGD(
                        "OBIS",
                        "Parity error at character %d",
                        i);
                    return;
                }

                // mask out parity bit
                buf[i] &= 0x7f;
            }
            buf[len] = '\0';
            ESP_LOGV("OBIS", "Received: %s", buf);

            if (buf[0] != '/') {
                ESP_LOGD(
                    "OBIS",
                    "Invalid starting character (%02x)",
                    buf[0]);
                return;
            }

            char *ptr = strchr(buf, '\r');
            if (ptr == NULL) {
                ESP_LOGD(
                    "OBIS",
                    "Missing newline (last byte: %02x)",
                    buf[len-1]);
                return;
            }

            do {
                ptr++;
            } while (*ptr == '\r' || *ptr == '\n');

            char field[24];
            char value[64];
            int matched_len;
            while (*ptr != '!' && *ptr != '\0' && sscanf(ptr, OBIS_SCANF, field, value, &matched_len) == 2) {
                ESP_LOGD(
                    "OBIS",
                    "Found field %s with value %s",
                    field,
                    value);
                ptr += matched_len;
                for (const auto &req_field : this->fields) {
                    if (!req_field.compare(field)) {
                        float fvalue;
                        if (sscanf(value, "%f", &fvalue) != 1) {
                            ESP_LOGD(
                                "OBIS",
                                "Cannot convert value %s of field %s to float",
                                value,
                                field
                            );
                        }
                        this->sensors[req_field]->publish_state(fvalue);
                    }
                }
            }
            if (!sscanf(ptr, "!%*[\r\n]")) {
                ESP_LOGD(
                    "OBIS",
                    "Missing ! terminator line");
            }
            if (*ptr != '\0') {
                ESP_LOGD(
                    "OBIS",
                    "Remaining message: %s",
                    ptr);
            }
        }
};
