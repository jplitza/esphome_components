import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import CONF_PIN, CONF_ID

CONF_DEBOUNCE = "debounce"

ftc532_ns = cg.esphome_ns.namespace("ftc532")
CONF_FTC532_ID = "ftc532_id"
FTC532Component = ftc532_ns.class_("FTC532Component", cg.Component)

AUTO_LOAD = ["binary_sensor"]

MULTI_CONF = True
CONFIG_SCHEMA = cv.COMPONENT_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(FTC532Component),
        cv.Required(CONF_PIN): pins.gpio_input_pin_schema,
        cv.Optional(CONF_DEBOUNCE, default=0): cv.int_range(0, 7),
    }
)


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)

    cg.add(var.set_debounce(config[CONF_DEBOUNCE]))
