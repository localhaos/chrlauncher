"use strict";

(() => {
  if (window.__chrlauncherChatGptDownloadContent) {
    return;
  }

  window.__chrlauncherChatGptDownloadContent = true;

  function sendRuntimeMessage(message) {
    try {
      chrome.runtime.sendMessage(message, () => {
        // Intentionally ignore runtime errors. The page still keeps its native behavior.
        void chrome.runtime.lastError;
      });
    } catch (_) {}
  }

  function injectPageHook() {
    const script = document.createElement("script");
    script.src = chrome.runtime.getURL("chatgpt_download_page_hook.js");
    script.dataset.chrlauncherChatgptDownloadFix = "1";

    (document.documentElement || document.head || document.body).appendChild(script);

    script.addEventListener("load", () => script.remove(), { once: true });
  }

  window.addEventListener("message", (event) => {
    if (event.source !== window) {
      return;
    }

    const data = event.data;
    if (!data || data.__chrlauncherChatGptDownloadFix !== "1") {
      return;
    }

    if (data.type === "DIRECT_DOWNLOAD") {
      sendRuntimeMessage({
        type: "CHATGPT_DIRECT_DOWNLOAD",
        requestUrl: data.requestUrl || "",
        directUrl: data.directUrl || "",
        filename: data.filename || ""
      });
    }
  }, true);

  injectPageHook();
})();
