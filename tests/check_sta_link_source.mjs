#!/usr/bin/env node
// Source-contract regression: ESP-IDF STA/IP governs real TCP handling,
// ESPHome software flag only informs diagnostics. No ESP32 is simulated here.
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
const root = new URL("../", import.meta.url);
const read = name => readFileSync(new URL(name, root), "utf8");
const h = read("components/modbus_gateway/modbus_gateway.h");
const p = read("components/modbus_gateway/tcp_connection_policy.h");
const t = read("tests/test_tcp_connection_policy.cpp");

assert.match(h, /#include <esp_netif\.h>/);
assert.match(h, /#include <esp_wifi\.h>/);
assert.match(h, /esp_wifi_sta_get_ap_info\(&ap_info\) == ESP_OK/);
assert.match(h, /esp_netif_get_handle_from_ifkey\("WIFI_STA_DEF"\)/);
assert.match(h, /esp_netif_get_ip_info\(station, &state\.ipv4_info\) == ESP_OK/);
assert.match(h, /state\.has_ipv4 = state\.ipv4_info\.ip\.addr != 0U/);
assert.match(h, /tcp_connection_policy::station_link_ready\(associated, has_ipv4\)/);
assert.match(h, /observe_station_mismatch_\(station\)/);
assert.match(h, /tcp_status_mismatches_/);
assert.match(h, /tcp_offline_connect_skips_/);
assert.doesNotMatch(h, /WiFi\.softAPIP\(\)/,
  "Fallback AP is not a remote Modbus TCP route");
for (const key of ["wifi_esphome_connected", "wifi_sta_associated",
                   "wifi_sta_has_ipv4", "wifi_sta_ready", "wifi_sta_ip",
                   "tcp_socket_connected", "wifi_status_disagreements",
                   "tcp_connect_skipped_no_sta", "tcp_sta_link_losses"]) {
  assert.ok(h.includes(key), "Missing /connections key " + key);
}
const supervisor = h.split("void maintain_persistent_tcp_client_() {")[1]
  ?.split("static bool is_cacheable_read_request_")[0];
assert.ok(supervisor);
assert.match(supervisor, /const StationLinkState station = station_link_state_\(\)/);
assert.match(supervisor, /if \(!station\.ready\(\)\)/);
assert.doesNotMatch(supervisor, /if \(!wifi_ready\)/,
  "ESPHome's stale software flag must not close a real TCP connection");
const foreground = h.split("void start_client_transaction_(")[1]
  ?.split("void finish_client_transaction_(")[0];
assert.ok(foreground);
assert.ok(foreground.indexOf("if (!station.ready())") <
          foreground.indexOf("client_.connect(cfg_host_"),
          "No blocking network connect without a STA/IP");
assert.match(foreground, /fail_client_transaction_\("Wi-Fi station not ready/);
const manual = h.split("bool transact_tcp_(")[1]
  ?.split("uint16_t tid=transaction_id_")[0];
assert.ok(manual);
assert.ok(manual.indexOf("if (!station.ready())") <
          manual.indexOf("client_.connect(cfg_host_"));
assert.match(h, /uint32_t\(now - tcp_last_mismatch_report_ms_\) >= 30000U/);
assert.match(p, /inline bool station_link_ready\(bool station_associated, bool station_has_ipv4\)/);
assert.match(p, /inline bool status_disagrees\(/);
assert.match(t, /assert\(station_link_ready\(true, true\)\)/);
assert.match(t, /assert\(status_disagrees\(false, true, true\)\)/);
assert.match(h, /if \(client_txn_\.state != ClientTxnState::IDLE/);
assert.match(h, /cached_response_tmp_\.clear\(\)/);
assert.match(h, /client_rtu_reply_is_timely_/);

const jsonFn = h.split("String connections_json_() {")[1]
  ?.split("String frames_json_() {")[0];
assert.ok(jsonFn, "Missing /connections JSON method");
assert.match(jsonFn, /const StationLinkState sta = station_link_state_\(\);/);
assert.match(jsonFn, /out \+= station_ip_text_\(sta\);/);
assert.doesNotMatch(h, /local_wifi_ip_\s*\(/,
  "Deleted legacy helper must not be referenced by /connections");

console.log("PASS: STA connection gating, disagreement logs, AP-only exclusion, TCP + cache invariants");
