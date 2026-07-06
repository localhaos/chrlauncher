"use strict";

const DISABLED_HOST_SUFFIXES = [
  "facebook.com",
  "fbcdn.net",
  "messenger.com"
];

function isDisabledHost(hostname) {
  const host = String(hostname || "").toLowerCase();
  return DISABLED_HOST_SUFFIXES.some((suffix) => host === suffix || host.endsWith("." + suffix));
}

function createLagGuardPanel() {
  chrome.devtools.panels.create(
    "Lag Guard",
    "",
    "devtools_panel.html",
    () => {}
  );
}

try {
  chrome.devtools.inspectedWindow.eval("location.hostname", { useContentScriptContext: false }, (hostname, exceptionInfo) => {
    if (exceptionInfo || !isDisabledHost(hostname)) {
      createLagGuardPanel();
    }
  });
} catch (_) {
  createLagGuardPanel();
}
