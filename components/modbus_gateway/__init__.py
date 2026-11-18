# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Modbus RTU-TCP Gateway contributors.
import json
import gzip
import re
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
    original_serve_local = web_server.serve_local
    script = r"""
<script>
(() => {
  // Match the URL shown by the Interface web text sensor. Do not depend
  // on the frontend's precise DOM markup or its unit formatting.
  const gatewayURL = location.protocol + "//" + location.hostname + "/";
  function walk(root) {
    const iterator = document.createTreeWalker(root, NodeFilter.SHOW_TEXT);
    const nodes = [];
    while (iterator.nextNode()) {
      const node = iterator.currentNode;
      const parent = node.parentElement;
      if (!parent || parent.closest("a,script,style,textarea")) continue;
      if (node.nodeValue.includes(gatewayURL)) nodes.push(node);
    }
    for (const node of nodes) {
      const value = node.nodeValue;
      const pos = value.indexOf(gatewayURL);
      if (pos < 0) continue;
      const before = document.createTextNode(value.slice(0, pos));
      const link = document.createElement("a");
      link.href = gatewayURL;
      link.target = "_blank";
      link.rel = "noopener noreferrer";
      link.textContent = gatewayURL;
      link.style.cssText = "color:#42a5f5;text-decoration:underline;cursor:pointer";
      const after = document.createTextNode(value.slice(pos + gatewayURL.length));
      node.replaceWith(before, link, after);
    }
    for (const el of root.querySelectorAll("*")) {
      if (el.shadowRoot) walk(el.shadowRoot);
    }
  }
  function refresh() { walk(document); }
  document.addEventListener("DOMContentLoaded", refresh);
  // ESPHome's UI is reactive; the text sensor may arrive after page load.
  setInterval(refresh, 750);
})();
</script>"""

    def _offline_v3_html():
        # ESPHome local:true serves a precompressed INDEX_GZ/INDEX_BR, bypassing
        # build_index_html entirely. Read ESPHome's own bundled v3 gzip frontend
        # at compile time and emit a patched, self-contained HTML resource.
        header = Path(web_server.__file__).with_name("server_index_v3.h")
        source = header.read_text(encoding="utf-8")
        match = re.search(
            r"#ifdef USE_WEBSERVER_GZIP\\s+constexpr uint8_t INDEX_GZ\\[\\] PROGMEM = \\{(.*?)\\};",
            source,
            flags=re.S,
        )
        if match is None:
            raise ValueError("Cannot locate ESPHome v3 bundled gzip frontend: " + str(header))
        packed = bytes(int(x, 16) for x in re.findall(r"0x([0-9a-fA-F]{2})", match.group(1)))
        html = gzip.decompress(packed).decode("utf-8")
        if "</body>" not in html:
            raise ValueError("ESPHome v3 bundled frontend has no closing body tag")
        return html.replace("</body>", script + "</body>", 1)

    def serve_local_with_link(config, wifi_config):
        if config.get("version") == 3 and config.get("local") is True:
            # Use ESPHome's ordinary INDEX_HTML path instead of INDEX_GZ so
            # the injected script is actually served. Everything is still
            # embedded in firmware; no browser Internet access is required.
            return False
        return original_serve_local(config, wifi_config)

    def build_index_html_with_link(config):
        if config.get("version") == 3 and config.get("local") is True:
            return _offline_v3_html()
        return original(config)

    build_index_html_with_link._modbus_gateway_link = True
    web_server.serve_local = serve_local_with_link
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
