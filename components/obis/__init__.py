import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

obis_ns = cg.esphome_ns.namespace("obis")
CONF_OBIS_ID = "obis_id"
OBISComponent = obis_ns.class_("OBISComponent", cg.Component)

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor"]

MULTI_CONF = True
CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID() : cv.declare_id(OBISComponent),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
)


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield uart.register_uart_device(var, config)
