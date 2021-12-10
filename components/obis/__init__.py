import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import (
    CONF_ID,
    CONF_FORCE_UPDATE,
    CONF_PAYLOAD,
    CONF_INTERVAL,
)

obis_ns = cg.esphome_ns.namespace("obis")
CONF_OBIS_ID = "obis_id"
OBISComponent = obis_ns.class_("OBISComponent", cg.Component)
PollingOBISComponent = obis_ns.class_("PollingOBISComponent", cg.Component)

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor"]

MULTI_CONF = True
CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(OBISComponent),
            cv.Optional(CONF_FORCE_UPDATE): cv.Schema({
                cv.Required(CONF_PAYLOAD): cv.string,
                cv.Required(CONF_INTERVAL): cv.int_range(1),
            }),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
)


def to_code(config):
    if CONF_FORCE_UPDATE in config:
        config[CONF_ID].type = PollingOBISComponent
        var = cg.new_Pvariable(
            config[CONF_ID],
            config[CONF_FORCE_UPDATE][CONF_INTERVAL],
        )
    else:
        var = cg.new_Pvariable(config[CONF_ID])

    yield cg.register_component(var, config)
    yield uart.register_uart_device(var, config)

    if CONF_FORCE_UPDATE in config:
        cg.add(var.set_update_payload(config[CONF_FORCE_UPDATE][CONF_PAYLOAD]))
