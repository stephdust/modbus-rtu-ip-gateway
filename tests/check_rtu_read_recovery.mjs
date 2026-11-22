#!/usr/bin/env node
// Static non-regression checks for the read-only SWR overload fix.
// Host C++ policy tests are in tests/test_gateway_policy.cpp.
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
const root = new URL("../", import.meta.url);
const read = name => readFileSync(new URL(name, root), "utf8");
const h = read("components/modbus_gateway/modbus_gateway.h");
const p = read("components/modbus_gateway/modbus_gateway_policy.h");
const oldHtmlMarker = "</div></main></body></html>)HTML";
const manifest = JSON.parse(read("components/modbus_gateway/manifest.json"));
assert.equal(manifest.version, "1.3.15");

// Especially important: 1.3.14 fixed a broken Web interface by streaming
// the unchanged 68KB static HTML. Do not accidentally undo that repair.
assert.match(h, /void send_page_\(\)/);
assert.doesNotMatch(h, /String page_\(\)/);
assert.match(h, /buffer\.reserve\(1024U\)/);
assert.ok(h.includes('web_.sendContent(""); // Terminating chunk.'), "Explicit final HTML chunk missing");
assert.ok(h.includes(oldHtmlMarker), "Full page HTML ending missing");
const page = h.split("  void send_page_() {")[1]
  ?.split("  String connections_json_() {")[0];
assert.ok(page, "Web page streaming function missing");
const fragments = [...page.matchAll(/R"HTML\(([\s\S]*?)\)HTML"/g)]
  .map(match => match[1]);
assert.equal(fragments.length, 29);
const html = fragments.join("");
assert.equal(html.length, 68521);
assert.ok(html.endsWith("</body></html>"));
assert.ok(html.includes("Debug des trames"));
assert.ok(html.includes("Paramètres Modbus RTU"));
assert.ok(html.includes("Cache client : répondre immédiatement"));

assert.match(p, /inline bool expired_rtu_read\(/);
assert.match(p, /function >= 1U && function <= 4U/);
assert.match(p, /valid_rtu_crc\(frame, 8U\)/);
assert.match(p, /inline uint32_t swr_read_connect_timeout_ms\(/);
assert.match(p, /const uint32_t cap = half_window > 500U/);
assert.match(h, /purge_expired_rtu_reads_\(\);\s*parse_rtu_\(\);/);
const purge = h.split("void purge_expired_rtu_reads_() {")[1]
  ?.split("void parse_rtu_() {")[0];
assert.ok(purge);
assert.match(purge, /if \(!swr_client_mode_\(\) \|\| rx_\.size\(\) < 8U/);
assert.match(purge, /rx_timestamps_us_\.size\(\) != rx_\.size\(\)/);
assert.match(purge, /while \(removed < 16U/);
assert.match(purge, /discardable_expired_read\(/);
assert.match(purge, /add_log_\("RTU RX", rx_\.data\(\), 8U\)/);
assert.doesNotMatch(purge, /start_client_transaction_\(/);
assert.doesNotMatch(purge, /send_rtu_\(/);
const parser = h.split("void parse_rtu_() {")[1]
  ?.split("void schedule_client_cache_refresh_() {")[0];
assert.ok(parser);
assert.ok(parser.indexOf("expired_rtu_read(") <
          parser.indexOf("cache_lookup_("));
const foreground = h.split("void start_client_transaction_(")[1]
  ?.split("void finish_client_transaction_() {")[0];
assert.ok(foreground);
assert.match(foreground, /!background_refresh && cacheable_read/);
assert.match(foreground, /pdu\[0\] >= 1U && pdu\[0\] <= 4U/);
assert.match(foreground, /swr_read_connect_timeout_ms\(/);
assert.match(foreground, /client_\.connect\(cfg_host_\.c_str\(\), cfg_port_,\s*int32_t\(connect_timeout_ms\)\)/);
assert.match(foreground, /if \(foreground_swr_read && gateway_policy::expired_rtu_read/);
assert.ok(foreground.indexOf("if (foreground_swr_read && gateway_policy::expired_rtu_read(") <
          foreground.indexOf("client_.write(request, request_length)"));
const process = h.split("void process_client_transaction_() {")[1]
  ?.split("bool read_exact_(")[0];
assert.ok(process);
assert.match(process, /swr_client_mode_\(\) && !client_txn_\.background_refresh/);
assert.match(process, /client_txn_\.cacheable_read/);
assert.match(process, /client_txn_\.background_refresh = true/);
assert.match(process, /expired_rtu_read\(/);
assert.match(process, /size_t budget = 512/);
assert.doesNotMatch(process.slice(0, process.indexOf("size_t budget = 512")),
                    /client_\.stop\(\)/);
assert.match(h, /rtu_expired_queued_reads/);
assert.match(h, /rtu_expired_foreground_reads/);
assert.match(h, /rtu_pending_bytes/);
assert.match(h, /client_rtu_reply_is_timely_\(/);
assert.match(h, /cache_invalidate_after_write_\(/);
assert.match(h, /retry_disconnected_read_\(/);
console.log("PASS: bounded FC01-04 reads, writes intact, FIFO + cache, full streamed Web UI");
