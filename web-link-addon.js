// ESPHome web_server v3 add-on: turn the gateway's text-sensor URL into a link.
// This module runs alongside ESPHome's default UI; it does not replace it.
// Web components may render their content inside nested shadow roots.
const gatewayUrl = `http://${location.hostname}/`;
function linkify(root) {
  const walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT);
  const nodes = [];
  while (walker.nextNode()) {
    const node = walker.currentNode;
    const parent = node.parentElement;
    if (!parent || parent.closest('a,script,style,input,textarea')) continue;
    if (node.nodeValue.trim() === gatewayUrl) nodes.push(node);
  }
  for (const node of nodes) {
    const link = document.createElement('a');
    link.href = gatewayUrl;
    link.target = '_blank';
    link.rel = 'noopener noreferrer';
    link.textContent = gatewayUrl;
    link.title = 'Open Modbus Gateway';
    link.style.cssText = 'color:#2196f3;text-decoration:underline;overflow-wrap:anywhere';
    node.replaceWith(link);
  }
  for (const element of root.querySelectorAll('*')) {
    if (element.shadowRoot) linkify(element.shadowRoot);
  }
}
// ESPHome updates entities dynamically. A low-frequency scan avoids mutation
// observer loops and remains independent of its internal component markup.
linkify(document);
setInterval(() => linkify(document), 1500);
