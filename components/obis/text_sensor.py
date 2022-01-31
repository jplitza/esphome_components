import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, CONF_CHANNEL
from . import obis_ns, OBISComponent, CONF_OBIS_ID

DEPENDENCIES = ["obis"]
OBISTextChannel = obis_ns.class_("OBISTextChannel", text_sensor.TextSensor)

CONFIG_SCHEMA = text_sensor.TEXT_SENSOR_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(OBISTextChannel),
    cv.GenerateID(CONF_OBIS_ID): cv.use_id(OBISComponent),
    cv.Required(CONF_CHANNEL): cv.string,
})


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield text_sensor.register_text_sensor(var, config)
    hub = yield cg.get_variable(config[CONF_OBIS_ID])
    cg.add(var.set_channel(config[CONF_CHANNEL]))
    cg.add(hub.register_channel(var))
