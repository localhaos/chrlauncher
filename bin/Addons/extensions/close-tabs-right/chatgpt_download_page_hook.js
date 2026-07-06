"use strict";

(() => {
  if (window.__chrlauncherChatGptDownloadPageHook) {
    return;
  }

  window.__chrlauncherChatGptDownloadPageHook = true;

  function filenameFromRequestUrl(requestUrl) {
    try {
      const parsed = new URL(requestUrl, location.href);
      const sandboxPath = parsed.searchParams.get("sandbox_path") || "";
      const piece = sandboxPath.split("/").filter(Boolean).pop();
      return piece ? decodeURIComponent(piece) : "";
    } catch (_) {
      return "";
    }
  }

  function postCapture(requestUrl, directUrl, source) {
    if (!requestUrl || !directUrl || !/^https?:\/\//i.test(directUrl)) {
      return;
    }

    window.postMessage({
      __chrlauncherChatGptDownloadFix: "1",
      type: "DIRECT_DOWNLOAD",
      source,
      requestUrl,
      directUrl,
      filename: filenameFromRequestUrl(requestUrl)
    }, "*");
  }

  function looksLikeChatGptDownloadPrep(url) {
    try {
      const parsed = new URL(url, location.href);
      const path = parsed.pathname.toLowerCase();

      if (path.includes("/interpreter/download")) {
        return true;
      }

      if (path.includes("/download") && (parsed.searchParams.has("sandbox_path") || parsed.searchParams.has("message_id"))) {
        return true;
      }

      return false;
    } catch (_) {
      return false;
    }
  }

  function directUrlFromPayload(payload) {
    if (!payload || typeof payload !== "object") {
      return "";
    }

    if (typeof payload.download_url === "string" && /^https?:\/\//i.test(payload.download_url)) {
      return payload.download_url;
    }

    if (typeof payload.url === "string" && /^https?:\/\//i.test(payload.url)) {
      return payload.url;
    }

    for (const key of ["data", "result", "file", "download"]) {
      if (payload[key] && typeof payload[key] === "object") {
        const nested = directUrlFromPayload(payload[key]);
        if (nested) {
          return nested;
        }
      }
    }

    return "";
  }

  async function inspectFetchResponse(requestUrl, response) {
    try {
      if (!response || typeof response.clone !== "function") {
        return;
      }

      const text = await response.clone().text();
      if (!text) {
        return;
      }

      let payload;
      try {
        payload = JSON.parse(text);
      } catch (_) {
        return;
      }

      const directUrl = directUrlFromPayload(payload);
      if (directUrl) {
        postCapture(requestUrl, directUrl, "fetch");
      }
    } catch (_) {}
  }

  function inspectXhrResponse(requestUrl, xhr) {
    try {
      let payload = null;

      if (xhr.responseType === "json" && xhr.response) {
        payload = xhr.response;
      } else if (!xhr.responseType || xhr.responseType === "text") {
        payload = JSON.parse(xhr.responseText || "null");
      }

      const directUrl = directUrlFromPayload(payload);
      if (directUrl) {
        postCapture(requestUrl, directUrl, "xhr");
      }
    } catch (_) {}
  }

  function requestUrlFromFetchInput(input) {
    if (typeof input === "string") {
      return input;
    }

    if (input && typeof input.url === "string") {
      return input.url;
    }

    return "";
  }

  const originalFetch = window.fetch;
  if (typeof originalFetch === "function") {
    window.fetch = function (...args) {
      const requestUrl = requestUrlFromFetchInput(args[0]);
      const result = originalFetch.apply(this, args);

      if (requestUrl && looksLikeChatGptDownloadPrep(requestUrl)) {
        Promise.resolve(result)
          .then((response) => inspectFetchResponse(requestUrl, response))
          .catch(() => {});
      }

      return result;
    };
  }

  const originalOpen = XMLHttpRequest.prototype.open;
  const originalSend = XMLHttpRequest.prototype.send;

  XMLHttpRequest.prototype.open = function (method, url, ...rest) {
    this.__chrlauncherChatGptDownloadRequestUrl = String(url || "");
    return originalOpen.call(this, method, url, ...rest);
  };

  XMLHttpRequest.prototype.send = function (...args) {
    const requestUrl = this.__chrlauncherChatGptDownloadRequestUrl || "";

    if (requestUrl && looksLikeChatGptDownloadPrep(requestUrl)) {
      this.addEventListener("load", () => inspectXhrResponse(requestUrl, this), { once: true });
    }

    return originalSend.apply(this, args);
  };
})();
