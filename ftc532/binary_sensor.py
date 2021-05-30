import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_CHANNEL, CONF_ID
from . import (
    ftc532_ns,
    FTC532Component,
    CONF_FTC532_ID,
)

DEPENDENCIES = ["ftc532"]
FTC532Channel = ftc532_ns.class_("FTC532Channel", binary_sensor.BinarySensor)

CONFIG_SCHEMA = binary_sensor.BINARY_SENSOR_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(FTC532Channel),
    cv.GenerateID(CONF_FTC532_ID): cv.use_id(FTC532Component),
    cv.Required(CONF_CHANNEL): cv.int_range(min=0, max=7),
})


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield binary_sensor.register_binary_sensor(var, config)
    hub = yield cg.get_variable(config[CONF_FTC532_ID])
    cg.add(var.set_channel(config[CONF_CHANNEL]))
    cg.add(hub.register_channel(var))
