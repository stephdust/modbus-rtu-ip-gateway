#!/usr/bin/env node
/**
 * Regression guard: the HTML is streamed from a bounded buffer, not assembled
 * in one massive Arduino String. Run: node tests/check_webui_html_stream.mjs
 *
 * This verifies the source and models the wire-byte chunk assembly. It is not
 * a substitute for an ESPHome build and an on-device HTTP/browser test.
 */
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";

const root = new URL("../", import.meta.url);
const read = p => readFileSync(new URL(p, root), "utf8");
const gateway = read("components/modbus_gateway/modbus_gateway.h");
const manifest = JSON.parse(read("components/modbus_gateway/manifest.json"));

assert.equal(manifest.version, "1.3.14");
assert.match(gateway, /web_\.on\("\/", HTTP_GET, \[this\]\(\) \{ this->send_page_\(\); \}\);/);
assert.doesNotMatch(gateway, /String page_\(\)/);
assert.doesNotMatch(gateway, /s\.reserve\(19000\)/);

const page = gateway.split("  void send_page_() {")[1]
  ?.split("  String connections_json_() {")[0];
assert.ok(page, "Streaming Web page method not found");
assert.match(page, /struct ChunkWriter \{/);
assert.match(page, /String buffer;/);
assert.match(page, /failed = !buffer\.reserve\(1024U\);/);
assert.match(page, /const size_t space = 1024U - buffer\.length\(\);/);
assert.match(page, /const size_t amount = length < space \? length : space;/);
assert.match(page, /buffer\.concat\(data, static_cast<unsigned int>\(amount\)\)/);
assert.match(page, /http\.sendContent\(buffer\);/);
assert.match(page, /web_\.sendHeader\("Cache-Control", "no-store, no-cache, must-revalidate"\)/);
assert.match(page, /web_\.setContentLength\(CONTENT_LENGTH_UNKNOWN\);/);
assert.match(page, /web_\.send\(200, "text\/html; charset=utf-8", ""\);/);
assert.ok(page.includes('s.flush();\n    web_.sendContent(""); // Terminating chunk.'), "HTTP final chunk missing");
assert.match(page, /web_\.setContentLength\(CONTENT_LENGTH_NOT_SET\);/);
assert.match(page, /HTML chunk allocation failed/);

const fragments = [...page.matchAll(/R"HTML\(([\s\S]*?)\)HTML"/g)].map(x => x[1]);
assert.equal(fragments.length, 29, "A raw HTML fragment was lost");
const staticHtml = fragments.join("");
assert.equal(staticHtml.length, 68521, "Full static HTML must be preserved");
assert.match(staticHtml, /^<!doctype html>/);
assert.ok(staticHtml.endsWith("</body></html>"));
for (const expected of [
  "Modbus IP", "Paramètres Modbus RTU", "Configuration",
  "État et connexions", "Compteurs / Diagnostics",
  "Debug des trames", "Cache client : répondre immédiatement",
  "Age", "Âge maximal des données", "Fenêtre maximale de réponse RTU",
  "form id=\"cfg\"", "bottom-actions", "function sendManual()"
]) {
  if (expected === "Age") continue;
  assert.ok(staticHtml.includes(expected), "Missing page element: " + expected);
}
assert.ok(staticHtml.indexOf("Paramètres Modbus RTU") > staticHtml.indexOf("Cache client : répondre immédiatement"));
assert.ok(staticHtml.indexOf("Debug des trames") > staticHtml.indexOf("Compteurs / Diagnostics"));

// Model the same 1024-byte boundary as the ESP32 String chunk writer, using
// UTF-8 bytes (String::length counts bytes, not JavaScript UTF-16 units).
const chunks = [];
let chunk = Buffer.alloc(0);
for (const fragment of fragments) {
  const bytes = Buffer.from(fragment, "utf8");
  let offset = 0;
  while (offset < bytes.length) {
    const space = 1024 - chunk.length;
    const take = Math.min(space, bytes.length - offset);
    chunk = Buffer.concat([chunk, bytes.subarray(offset, offset + take)]);
    offset += take;
    if (chunk.length === 1024) {
      chunks.push(chunk);
      chunk = Buffer.alloc(0);
    }
  }
}
if (chunk.length) chunks.push(chunk);
assert.ok(chunks.length > 65, "Page should be sent in many bounded chunks");
assert.ok(chunks.every(c => c.length > 0 && c.length <= 1024));
assert.equal(Buffer.concat(chunks).toString("utf8"), staticHtml);
console.log("PASS: complete HTML dashboard, 1024-byte chunking, explicit HTTP terminator, no giant String");
