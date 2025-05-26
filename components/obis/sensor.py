import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, CONF_CHANNEL
from . import obis_ns, OBISComponent, CONF_OBIS_ID

DEPENDENCIES = ["obis"]
OBISChannel = obis_ns.class_("OBISChannel", sensor.Sensor)

CONFIG_SCHEMA = sensor.sensor_schema().extend({
    cv.GenerateID(): cv.declare_id(OBISChannel),
    cv.GenerateID(CONF_OBIS_ID): cv.use_id(OBISComponent),
    cv.Required(CONF_CHANNEL): cv.string,
})


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield sensor.register_sensor(var, config)
    hub = yield cg.get_variable(config[CONF_OBIS_ID])
    cg.add(var.set_channel(config[CONF_CHANNEL]))
    cg.add(hub.register_channel(var))
