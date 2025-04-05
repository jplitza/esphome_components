import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID, CONF_TRIGGER_ID
from esphome import automation

obis_ns = cg.esphome_ns.namespace("obis")
CONF_OBIS_ID = "obis_id"
CONF_ON_OPENING = "on_opening"
CONF_ON_CLOSING = "on_closing"
OBISComponent = obis_ns.class_("OBISComponent", cg.Component)
OBISTrigger = obis_ns.class_("OBISTrigger", automation.Trigger.template())

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "text_sensor"]

MULTI_CONF = True
CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(OBISComponent),
            cv.Optional(CONF_ON_OPENING): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(OBISTrigger),
                }
            ),
            cv.Optional(CONF_ON_CLOSING): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(OBISTrigger),
                }
            ),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
)


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield uart.register_uart_device(var, config)

    for conf in config.get(CONF_ON_OPENING, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.register_opening_trigger(trigger))
        yield automation.build_automation(trigger, [], conf)

    for conf in config.get(CONF_ON_CLOSING, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.register_closing_trigger(trigger))
        yield automation.build_automation(trigger, [], conf)
