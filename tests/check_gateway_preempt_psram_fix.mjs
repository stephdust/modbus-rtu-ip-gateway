// node tests/check_gateway_preempt_psram_fix.mjs
// Structural source regression: no accidental response/TID alias, no
// read-preempt close, write priority preserved, 256-frame PSRAM ring,
// chunked (valid escaped) JSON and monitor/export endpoints unchanged.
import fs from "node:fs";
import assert from "node:assert/strict";
const s=fs.readFileSync("components/modbus_gateway/modbus_gateway.h","utf8");
const has=(x)=>assert.ok(s.includes(x),"missing: "+x);
for(const x of [
  '#include "deferred_rtu_policy.h"',
  '#include "frame_log_policy.h"',
  'frame_logs_ = static_cast<FrameLog *>(heap_caps_calloc(',
  "MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT",
  "FRAME_LOG_CAPACITY = 256",
  "FRAME_WEB_VISIBLE = 64",
  "FrameLog *frame_logs_{nullptr}",
  "frame_log_policy::encode_hex(",
  "frame_log_advance_(entry, direction)",
  "client_txn_.background_refresh, false, pdu[0])",
  "deferred_fg_.active = true",
  "deferred_fg_.received_us = end_to_end_started_us",
  "deferred_fg_.size = uint16_t(pdu.size())",
  "void service_deferred_foreground_()",
  "deferred_rtu_policy::read_expired(",
  "deferred_rtu_policy::discard_for_new_master(",
  "client_txn_.state == ClientTxnState::IDLE",
  "deferred_fg_.active = false",
  'close_tcp_client_diagnostic_("background_preempted_by_write")',
  'web_.on("/frames", HTTP_GET, [this]() { this->send_frames_json_(); });',
  "web_.setContentLength(CONTENT_LENGTH_UNKNOWN)",
  "append_json_escaped_(row, entry.hex)",
  "frame_logs_[idx].hex",
  "frame_log_psram_bytes",
  "deferred_foreground_dispatched",
  "deferred_read_expired",
  "DIAG DEFER EXPIRED",
  "DIAG DEFER SEND",
  "DIAG PREEMPT WRITE",
  'web_.on("/frames.txt"',
  'web_.on("/diag.txt"'
])has(x);
assert.doesNotMatch(s,/FrameLog logs_\[/);
assert.doesNotMatch(s,/String frames_json_\(/);
assert.doesNotMatch(s,/close_tcp_client_diagnostic_\("background_preempted"\)/);
const preempt=s.slice(s.indexOf("if (client_txn_.state != ClientTxnState::IDLE &&\n          client_txn_.background_refresh &&\n          !deferred_rtu_policy::is_read(pdu[0]))"), s.indexOf("// Start the foreground TCP exchange"));
assert.match(preempt,/!deferred_rtu_policy::is_read/);
assert.match(preempt,/DIAG PREEMPT WRITE/);
assert.match(preempt,/background_preempted_by_write/);
assert.match(preempt,/else if \(deferred_rtu_policy::should_defer/);
assert.match(preempt,/deferred_fg_\.active = true/);
assert.doesNotMatch(preempt,/close_tcp_client_diagnostic_\("background_preempted"\)/);
const service=s.slice(s.indexOf("void service_deferred_foreground_()"),s.indexOf("// Select the most overdue"));
assert.match(service,/deferred_rtu_policy::can_dispatch/);
assert.match(service,/record_expired_queued_read_/);
assert.match(service,/start_client_transaction_\(unit, pdu, received_us, cacheable\)/);
assert.doesNotMatch(service,/close_tcp_client_diagnostic_/);
const capture=s.slice(s.indexOf("void add_log_("),s.indexOf("void enable_client_tcp_keepalive_()"));
assert.doesNotMatch(capture,/String h;/);
assert.match(capture,/snprintf\(entry\.hex, sizeof\(entry\.hex\)/);
assert.match(capture,/frame_log_policy::encode_hex/);
assert.equal((s.match(/Serial1\.write\(/g)||[]).length,3);
assert.equal((s.match(/esp_event_handler_instance_register\(/g)||[]).length,4);
assert.equal((s.match(/close_tcp_client_diagnostic_\("/g)||[]).length,23);
console.log("PASS: foreground read deferral, write preempt, TID isolation, PSRAM frame log, 64/256 API compatibility");
