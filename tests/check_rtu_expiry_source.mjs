#!/usr/bin/env node
/**
 * Source-contract regression for bounded read-only RTU backlog cleanup.
 * Host-side logical behavior is covered in test_gateway_policy.cpp.
 */
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";

const root = new URL("../", import.meta.url);
const read = path => readFileSync(new URL(path, root), "utf8");
const gateway = read("components/modbus_gateway/modbus_gateway.h");
const policy = read("components/modbus_gateway/modbus_gateway_policy.h");
const tests = read("tests/test_gateway_policy.cpp");

const loop = gateway.split("void loop() override {")[1]
  ?.split("float get_setup_priority()")[0];
assert.ok(loop, "Gateway loop missing");
assert.match(loop, /purge_expired_rtu_read_backlog_\(\);\s*parse_rtu_\(\);/);
assert.ok(loop.indexOf("while (Serial1.available())") <
          loop.indexOf("purge_expired_rtu_read_backlog_();"),
          "Read bytes and timestamps from UART before pruning");

const prune = gateway.split("void purge_expired_rtu_read_backlog_() {")[1]
  ?.split("void parse_rtu_() {")[0];
assert.ok(prune, "Read backlog purge missing");
assert.match(prune, /if \(!swr_client_mode_\(\) \|\| rx_\.size\(\) < 8U/);
assert.match(prune, /rx_timestamps_us_\.size\(\) != rx_\.size\(\)/);
assert.match(prune, /while \(dropped < 16U && rx_\.size\(\) >= 8U\)/);
assert.match(prune, /discardable_expired_rtu_read\(/);
assert.match(prune, /add_log_\("RTU RX", rx_\.data\(\), 8U\);/);
assert.match(prune, /rx_\.erase\(rx_\.begin\(\), rx_\.begin\(\) \+ 8U\);/);
assert.match(prune, /rx_timestamps_us_\.erase\(rx_timestamps_us_\.begin\(\)/);
assert.match(prune, /uint32_t\(now_ms - rtu_last_expired_report_ms_\) >= 5000U/);
assert.doesNotMatch(prune, /start_client_transaction_\(/);
assert.doesNotMatch(prune, /send_rtu_\(/);

const process = gateway.split("void process_client_transaction_() {")[1]
  ?.split("bool read_exact_(")[0];
assert.ok(process, "Async TCP state machine missing");
const expiry = process.split("if (swr_client_mode_() && !client_txn_.background_refresh &&")[1]
  ?.split("size_t budget = 512;")[0];
assert.ok(expiry, "Missing foreground read-only expiry guard");
assert.match(expiry, /client_txn_\.request_pdu\.size\(\) == 5U/);
assert.match(expiry, /gateway_policy::read_reply_window_expired\(/);
assert.match(expiry, /client_\.stop\(\);/);
assert.match(expiry, /reset_client_transaction_\(\);/);
assert.match(expiry, /return;/);
assert.doesNotMatch(expiry, /retry_disconnected_read_\(/);

assert.match(policy, /function_code >= 1U && function_code <= 4U/);
assert.match(policy, /uint64_t\(elapsed_us\) >= uint64_t\(window_ms\) \* 1000ULL/);
assert.match(policy, /head\[0\] >= 1U && head\[0\] <= 247U/);
assert.match(policy, /valid_rtu_crc\(head, 8U\)/);
for (const fc of [6, 16, 23]) {
  assert.ok(tests.includes("queued" + fc),
    "Host test must preserve FC" + fc + " writes");
}
assert.match(tests, /micros\(\) rollover/);
assert.match(gateway, /client_rtu_reply_is_timely_\(/);
assert.match(gateway, /pending master bytes=/);
assert.match(gateway, /rtu_expired_queued_reads/);
assert.match(gateway, /rtu_expired_foreground_reads/);
assert.match(gateway, /rtu_pending_bytes/);
assert.match(gateway, /uint32_t tcp_timeout_ms_\{3000\}/);
assert.match(gateway, /kBackgroundRefreshMinIntervalMs = 250U/);
console.log("PASS: stale RTU reads are suppressed safely, writes and TCP behavior preserved");
