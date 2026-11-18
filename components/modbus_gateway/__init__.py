# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Modbus RTU-TCP Gateway contributors.
import json
from pathlib import Path

import esphome.codegen as cg
from esphome.core import CORE
import esphome.config_validation as cv
from esphome.const import CONF_ID


# Add a small, firmware-embedded link enhancement to ESPHome's v3 page.
# The official frontend remains embedded by web_server.local: true; no CDN,
# manually copied JS file, or network request is required at runtime.
# This is deliberately limited to the HTML index builder, not the HTTP server.
def _install_offline_gateway_link():
    from esphome.components import web_server

    if getattr(web_server.build_index_html, "_modbus_gateway_link", False):
        return

    original = web_server.build_index_html
    script = r"""
<script>
(() => {
  const url = location.protocol + "//" + location.hostname + "/";
  function linkify(root) {
    const walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT);
    const matches = [];
    while (walker.nextNode()) {
      const node = walker.currentNode;
      if (node.nodeValue.trim() !== url) continue;
      const parent = node.parentElement;
      if (!parent || parent.closest("a,script,style,textarea")) continue;
      matches.push(node);
    }
    for (const node of matches) {
      const a = document.createElement("a");
      a.href = url;
      a.target = "_blank";
      a.rel = "noopener noreferrer";
      a.textContent = url;
      a.style.cssText = "color:#2196f3;text-decoration:underline;overflow-wrap:anywhere";
      node.replaceWith(a);
    }
    for (const el of root.querySelectorAll("*")) {
      if (el.shadowRoot) linkify(el.shadowRoot);
    }
  }
  const update = () => linkify(document);
  if (document.readyState === "loading")
    document.addEventListener("DOMContentLoaded", update, {once:true});
  else update();
  setInterval(update, 1500);
})();
</script>"""

    def build_index_html_with_link(config):
        html = original(config)
        if config.get("version") != 3:
            return html
        return html.replace("</body>", script + "</body>", 1)

    build_index_html_with_link._modbus_gateway_link = True
    web_server.build_index_html = build_index_html_with_link


_install_offline_gateway_link()

gateway_ns = cg.esphome_ns.namespace("modbus_gateway")
ModbusGateway = gateway_ns.class_("ModbusGateway", cg.Component)

CONF_TCP_HOST = "tcp_host"
CONF_TCP_MODE = "tcp_mode"
CONF_TRANSPORT = "transport"
CONF_TCP_PORT = "tcp_port"
CONF_BAUD = "baud"
CONF_DATA_BITS = "data_bits"
CONF_PARITY = "parity"
CONF_STOP_BITS = "stop_bits"
CONF_TCP_TIMEOUT = "tcp_timeout"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(ModbusGateway),
    cv.Optional(CONF_TCP_HOST, default="192.168.1.100"): cv.string,
    cv.Optional(CONF_TCP_MODE, default="CLIENT"): cv.one_of("CLIENT", "SERVER", upper=True),
    cv.Optional(CONF_TRANSPORT, default="TCP"): cv.one_of("TCP", "UDP", upper=True),
    cv.Optional(CONF_TCP_PORT, default=502): cv.port,
    cv.Optional(CONF_BAUD, default=9600): cv.int_range(min=1200, max=115200),
    cv.Optional(CONF_DATA_BITS, default=8): cv.one_of(7, 8, int=True),
    cv.Optional(CONF_PARITY, default="NONE"): cv.one_of("NONE", "EVEN", "ODD", upper=True),
    cv.Optional(CONF_STOP_BITS, default=1): cv.one_of(1, 2, int=True),
    cv.Optional(CONF_TCP_TIMEOUT, default="3s"): cv.positive_time_period_milliseconds,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    # Use the component manifest as the single source of truth for the Web UI version.
    manifest_path = Path(__file__).with_name("manifest.json")
    with manifest_path.open("r", encoding="utf-8") as manifest_file:
        component_version = json.load(manifest_file).get("version", "unknown")

    # ESPHome 2026.2+ selective Arduino compilation: explicitly enable
    # the Arduino libraries used by modbus_gateway.h.
    if CORE.is_esp32 and CORE.using_arduino:
        cg.add_library("WiFi", None)
        cg.add_library("Network", None)
        cg.add_library("WebServer", None)
        cg.add_library("Preferences", None)

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_version(component_version))
    cg.add(var.set_tcp_host(config[CONF_TCP_HOST]))
    cg.add(var.set_tcp_mode(config[CONF_TCP_MODE]))
    cg.add(var.set_transport(config[CONF_TRANSPORT]))
    cg.add(var.set_tcp_port(config[CONF_TCP_PORT]))
    cg.add(var.set_baud(config[CONF_BAUD]))
    cg.add(var.set_data_bits(config[CONF_DATA_BITS]))
    cg.add(var.set_parity(config[CONF_PARITY]))
    cg.add(var.set_stop_bits(config[CONF_STOP_BITS]))
    cg.add(var.set_tx_gpio(17))
    cg.add(var.set_rx_gpio(18))
    cg.add(var.set_de_re_gpio(21))
    cg.add(var.set_tcp_timeout(config[CONF_TCP_TIMEOUT].total_milliseconds))
