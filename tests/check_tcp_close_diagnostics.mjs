// node tests/check_tcp_close_diagnostics.mjs
// Source-contract regression for the diagnostics-only ESPHome gateway patch.
// This deliberately does not simulate Wi-Fi, lwIP or real TCP timing.
import fs from "node:fs";
import assert from "node:assert/strict";

const file = fs.readFileSync("components/modbus_gateway/modbus_gateway.h", "utf8");
const section = (a,b) => {
  const start = file.indexOf(a);
  const end = file.indexOf(b, start+a.length);
  assert.ok(start >= 0 && end > start, "missing section " + a);
  return file.slice(start,end);
};
const policy = fs.readFileSync("components/modbus_gateway/tcp_connection_policy.h", "utf8");
assert.match(policy, /return station_associated && station_has_ipv4/);
assert.match(file, /esp_wifi_sta_get_ap_info\(&ap_info\) == ESP_OK/);
assert.match(file, /esp_netif_get_handle_from_ifkey\("WIFI_STA_DEF"\)/);
assert.match(file, /esp_event_handler_instance_register\(\s*WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED/);
assert.match(file, /esp_event_handler_instance_register\(\s*IP_EVENT, IP_EVENT_STA_LOST_IP/);
assert.match(file, /WIFI STA DOWN/);
assert.match(file, /WIFI STA IP LOST/);
assert.match(file, /esp_reason=/);
assert.match(file, /driver_assoc=/);
assert.match(file, /sta_ipv4=/);
assert.match(file, /esphome_sw=/);
assert.match(file, /TCP OPEN/);
assert.match(file, /TCP CLOSE/);
assert.match(file, /local_port=/);
assert.match(file, /sid=/);
assert.match(file, /fd=/);
assert.match(file, /cache_refresh=/);
assert.match(file, /String\("reason="\) \+ reason/);
const disconnector = section("static void on_sta_disconnected_diag_", "static void on_sta_lost_ip_diag_");
assert.doesNotMatch(disconnector, /client_\.stop\(|add_event_\(|String\(|tcp_link_was_up_/);
assert.match(disconnector, /__atomic_store_n/);
assert.match(disconnector, /__atomic_add_fetch/);
const ipHandler = section("static void on_sta_lost_ip_diag_", "void report_ip_lost_diag_");
assert.doesNotMatch(ipHandler, /client_\.stop\(|add_event_\(|String\(/);
const closeMethod = section("void close_tcp_client_diagnostic_", "void maintain_persistent_tcp_client_");
assert.match(file, /::getsockname\(/);
assert.match(closeMethod, /client_\.stop\(\)/);
assert.match(closeMethod, /driver_assoc=/);
const nonHelper = file.replace(closeMethod,"");
assert.doesNotMatch(nonHelper, /\bclient_\.stop\(\)/,
  "Every gateway TCP stop must have one recorded cause");
assert.equal((file.match(/note_tcp_client_open_\("/g) || []).length, 3);
for(const cause of ["sta_unavailable","background_preempted_by_write","idle_peer_closed",
                    "read_retry_reconnect","transaction_failed","foreground_write_failed"]) {
  assert.ok(file.includes('close_tcp_client_diagnostic_("'+cause+'")'), cause);
}
console.log("PASS: 23 close causes (write preemption only), TCP source-port correlation, 3 opens, STA/IP/ESPHome event separation");
