// Incremental Web/API regression contract. Run: node tests/check_gateway_web_delta.mjs
import fs from "node:fs";
import assert from "node:assert/strict";
import vm from "node:vm";
const source=fs.readFileSync("components/modbus_gateway/modbus_gateway.h","utf8");
const policy=fs.readFileSync("components/modbus_gateway/frame_delta_policy.h","utf8");
const requireSource=(token)=>assert.ok(source.includes(token),"missing: "+token);
for(const token of [
  '#include "frame_delta_policy.h"',
  'frame_stream_epoch_ = esp_random()',
  'frame_delta_policy::parse_u32(web_.arg("after").c_str(), after)',
  'frame_delta_policy::parse_u32(web_.arg("epoch").c_str(), epoch)',
  'frame_delta_policy::Selection choice',
  'entry.seq = frame_log_sequence_',
  'if (!frame_log_sequence_)',
  '++frame_stream_epoch_',
  'diag_frames_cursor_resyncs_',
  'frames_delta_responses',
  'frames_empty_responses',
  'frames_rows_sent',
  'row.reserve(1152U)',
  'chunk += "],\\\"epoch\\\":";',
  'chunk += ",\\\"seq\\\":";',
  'chunk += ",\\\"reset\\\":";',
  'frameEpoch=null,frameCursor=0,frameHistory=[]',
  'if(frameRefreshBusy)return',
  'frameHistory=frameHistory.concat(frames).slice(-64)',
  'if(reset)frameHistory=frames.slice(-64)',
  'if(requestGeneration!==frameRequestGeneration)return',
  "document.addEventListener('visibilitychange'",
  'frame_log_psram',
  'void send_frames_text_()',
  'web_.on("/diag.txt"'
])requireSource(token);
assert.match(policy,/inline Selection select/);
assert.match(policy,/inline bool parse_u32/);
const script=source.match(/<script>([\s\S]*?)<\/script>/);
assert.ok(script && script[1].includes("function refresh()"));
new vm.Script(script[1],{filename:"gateway-embedded-web.js"});
assert.equal((source.match(/Serial1\.write\(/g)||[]).length,3);
assert.equal((source.match(/esp_event_handler_instance_register\(/g)||[]).length,4);
assert.ok(source.includes('web_.on("/frames", HTTP_GET, [this]() { this->send_frames_json_(); });'));
assert.ok(!source.includes("String frames_json_()"));
console.log("PASS: incremental cursor API, JS syntax, clear/restart recovery, Web/RTU/diagnostic contracts");
