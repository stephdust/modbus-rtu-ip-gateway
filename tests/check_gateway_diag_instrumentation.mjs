// node tests/check_gateway_diag_instrumentation.mjs
// Source-contract regression: diagnostics must never alter RTU framing,
// scheduler, Wi-Fi readiness decisions, socket close reasons or TCP counts.
import fs from "node:fs";
import assert from "node:assert/strict";

const root = "components/modbus_gateway/";
const src = fs.readFileSync(root + "modbus_gateway.h", "utf8");
const header = fs.readFileSync(root + "gateway_diagnostic_policy.h", "utf8");
const count = (expression) => (src.match(expression) || []).length;
const must = (text) => assert.ok(src.includes(text), "missing source marker: " + text);

for (const token of [
  'web_.on("/diag.txt"', 'web_.on("/diag.json"',
  "heap_caps_calloc(", "MALLOC_CAP_SPIRAM", "kDiagCapacity = 128U",
  'diag_event_("DIAG HEARTBEAT"', 'diag_event_("DIAG STAGES"',
  'diag_event_("DIAG LOOP GAP"', 'diag_event_("DIAG STAGE SLOW"',
  'diag_event_("DIAG UART BACKLOG"', 'diag_event_("DIAG UART BUFFERED"',
  'diag_event_("DIAG RTU RX GAP"', 'diag_event_("DIAG UART TX SLOW"',
  'diag_event_("DIAG TCP WAIT"', 'diag_event_("DIAG CACHE SLOW"',
  'diag_event_("DIAG MONITOR GAP"', 'diag_event_("DIAG WEB RENDER"',
  'diag_event_("DIAG WEB EXPORT"', 'diag_event_("DIAG PREEMPT"',
  'DIAG RTU LISTEN', 'DIAG FRAME CLEAR', 'DIAG CONFIG',
  'WIFI_EVENT_STA_DISCONNECTED', 'WIFI_EVENT_STA_CONNECTED',
  'IP_EVENT_STA_LOST_IP', 'IP_EVENT_STA_GOT_IP',
  'sta_ipv4=', 'esphome_sw=', 'driver_assoc=',
  'local_port=', 'TCP CLOSE', 'TCP OPEN',
  "rtu_wire_time_us(", "likely_uart_buffered(", "unexpected_uart_tx("
]) must(token);

for (const stage of ["Web", "Network", "UartDrain", "RtuParse",
                     "CacheSchedule", "TcpSupervisor"]) {
  must("diag_stage_sample_(DiagStage::" + stage);
}
assert.equal(count(/esp_event_handler_instance_register\(/g), 4);
assert.equal(count(/note_tcp_client_open_\("/g), 3);
assert.equal(count(/close_tcp_client_diagnostic_\("/g), 23);
assert.equal(count(/diag_tcp_connect_\(uint32_t\(micros\(\) - diagConnectStartedUs\)/g), 3);
assert.equal(count(/diag_uart_transmit_\((frame|rtu)\.size\(\)/g), 3);
assert.equal(count(/Serial1\.write\(/g), 3);
const eventHandlers = src.slice(
  src.indexOf("static void on_sta_connected_diag_"),
  src.indexOf("void report_wifi_associated_diag_")
);
assert.doesNotMatch(eventHandlers, /client_\.stop\(\)|add_event_\(|diag_event_\(/);
assert.match(eventHandlers, /__atomic_add_fetch/);
const critical = src.slice(
  src.indexOf("void diag_stage_sample_"), src.indexOf("String diag_json_")
);
assert.doesNotMatch(critical, /heap_caps_calloc\(|delay\(|delayMicroseconds\(/);
must("return tcp_connection_policy::station_link_ready(associated, has_ipv4)");
assert.match(header, /namespace gateway_diagnostic_policy/);
console.log("PASS: 6 timed stages, UART/TCP/cache/Web fault markers, 4 Wi-Fi events,");
console.log("      3 connect paths, 3 UART TX paths, 23 close reasons, no frame-level diag spam.");
