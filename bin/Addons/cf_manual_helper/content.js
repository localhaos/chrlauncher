"use strict";

(() => {
  const ROOT_ID = "chrlauncher-cf-manual-helper-root";
  const HIDDEN_KEY = "chrlauncher_cf_manual_helper_hidden_until_reload";
  const STATE_KEY = "chrlauncher_cf_manual_helper_last_state";
  const SCAN_DELAY_MS = 750;
  const MAX_SCANS_AFTER_DETECTION = 8;

  if (window.__chrlauncherCfManualHelperLoaded) {
    return;
  }

  window.__chrlauncherCfManualHelperLoaded = true;

  let scanTimer = 0;
  let detectionCount = 0;
  let observer = null;

  function lower(value) {
    return String(value || "").toLowerCase();
  }

  function includesAny(value, needles) {
    const text = lower(value);
    return needles.some((needle) => text.includes(needle));
  }

  function visibleTextSample() {
    const body = document.body;
    if (!body) {
      return "";
    }

    return String(body.innerText || body.textContent || "").slice(0, 24000);
  }

  function collectSignals() {
    const signals = [];
    const title = document.title || "";
    const html = document.documentElement ? document.documentElement.innerHTML.slice(0, 240000) : "";
    const text = visibleTextSample();
    const metas = Array.from(document.querySelectorAll("meta[name],meta[http-equiv],meta[content]"))
      .map((meta) => [meta.name, meta.httpEquiv, meta.content].join(" "))
      .join("\n");
    const scripts = Array.from(document.scripts || [])
      .map((script) => script.src || "")
      .filter(Boolean)
      .join("\n");
    const iframes = Array.from(document.querySelectorAll("iframe[src]"))
      .map((frame) => frame.src || "")
      .join("\n");

    if (includesAny(title, ["just a moment", "attention required", "checking your browser", "security check"])) {
      signals.push("title: challenge-like title");
    }

    if (includesAny(text, [
      "checking if the site connection is secure",
      "verify you are human",
      "needs to review the security of your connection",
      "ray id",
      "performance & security by cloudflare",
      "cloudflare ray id",
      "enable javascript and cookies to continue"
    ])) {
      signals.push("body: challenge/interstitial text");
    }

    if (includesAny(html, ["cf-chl-widget", "cf-turnstile", "cf_clearance", "challenge-platform", "cdn-cgi/challenge-platform"])) {
      signals.push("dom: cloudflare challenge markers");
    }

    if (includesAny(metas, ["cloudflare", "cf-ray", "refresh"])) {
      signals.push("meta: cloudflare/refresh markers");
    }

    if (includesAny(scripts, ["/cdn-cgi/challenge-platform/", "challenges.cloudflare.com", "turnstile"])) {
      signals.push("script: challenge platform script");
    }

    if (includesAny(iframes, ["challenges.cloudflare.com", "turnstile"])) {
      signals.push("iframe: turnstile/challenge frame");
    }

    if (document.querySelector("input[name='cf-turnstile-response'], .cf-turnstile, [data-sitekey][data-action], [data-callback][data-sitekey]")) {
      signals.push("node: turnstile-like widget node");
    }

    return signals;
  }

  function hasClearanceCookie() {
    try {
      return document.cookie.split(";").some((entry) => entry.trim().startsWith("cf_clearance="));
    } catch (_) {
      return false;
    }
  }

  function getDiagnostics(signals) {
    return {
      url: location.href,
      host: location.hostname,
      title: document.title || "",
      time: new Date().toISOString(),
      signals,
      cfClearanceVisibleToPageScript: hasClearanceCookie(),
      userAgent: navigator.userAgent,
      cookiesEnabled: navigator.cookieEnabled,
      webdriver: Boolean(navigator.webdriver),
      visibilityState: document.visibilityState,
      note: "Manual diagnostics only. This helper does not solve, click, bypass, mirror requests, or generate clearance cookies."
    };
  }

  function saveState(state) {
    try {
      chrome.storage.local.set({ [STATE_KEY]: state });
    } catch (_) {}
  }

  function copyText(value) {
    const text = typeof value === "string" ? value : JSON.stringify(value, null, 2);

    if (navigator.clipboard && navigator.clipboard.writeText) {
      return navigator.clipboard.writeText(text);
    }

    const textarea = document.createElement("textarea");
    textarea.value = text;
    textarea.style.position = "fixed";
    textarea.style.opacity = "0";
    document.documentElement.appendChild(textarea);
    textarea.select();
    document.execCommand("copy");
    textarea.remove();
    return Promise.resolve();
  }

  function hiddenUntilReload() {
    try {
      return sessionStorage.getItem(HIDDEN_KEY) === "1";
    } catch (_) {
      return false;
    }
  }

  function setHiddenUntilReload() {
    try {
      sessionStorage.setItem(HIDDEN_KEY, "1");
    } catch (_) {}
  }

  function renderPanel(state) {
    if (hiddenUntilReload() || document.getElementById(ROOT_ID)) {
      return;
    }

    const host = document.createElement("div");
    host.id = ROOT_ID;
    host.style.cssText = "position:fixed;right:14px;bottom:14px;z-index:2147483647;all:initial";

    const shadow = host.attachShadow({ mode: "closed" });
    const wrapper = document.createElement("section");
    wrapper.innerHTML = `
      <style>
        :host { all: initial; }
        .box {
          width: min(420px, calc(100vw - 28px));
          box-sizing: border-box;
          font: 12px/1.45 ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
          color: #e5e7eb;
          background: #0f172a;
          border: 1px solid #334155;
          border-radius: 14px;
          box-shadow: 0 18px 60px rgba(0,0,0,.42);
          padding: 13px;
        }
        .title { font-weight: 800; font-size: 13px; margin-bottom: 7px; color: #fff; }
        .muted { color: #cbd5e1; margin: 0 0 8px; }
        .signals { margin: 8px 0; padding: 8px; background: #020617; border: 1px solid #1e293b; border-radius: 10px; }
        .signals div { color: #93c5fd; margin: 2px 0; }
        .row { display: flex; gap: 7px; flex-wrap: wrap; margin-top: 9px; }
        button {
          border: 1px solid #475569;
          border-radius: 8px;
          background: #1e293b;
          color: #f8fafc;
          cursor: pointer;
          padding: 6px 9px;
          font: inherit;
        }
        button:hover { border-color: #60a5fa; }
        .warn { color: #fde68a; }
      </style>
      <div class="box">
        <div class="title">chrlauncher CF Manual Helper</div>
        <p class="muted">Detected a Cloudflare/security challenge pattern on this page.</p>
        <p class="muted warn">Manual-session diagnostics only. No bypass, no auto-click, no cookie generation.</p>
        <div class="signals"></div>
        <p class="muted">cf_clearance visible to page script: <strong>${state.cfClearanceVisibleToPageScript ? "yes" : "no / HttpOnly / absent"}</strong></p>
        <div class="row">
          <button type="button" data-action="copy">Copy diagnostics</button>
          <button type="button" data-action="reload">Reload</button>
          <button type="button" data-action="hide">Hide until reload</button>
        </div>
      </div>
    `;

    const signals = wrapper.querySelector(".signals");
    for (const signal of state.signals) {
      const line = document.createElement("div");
      line.textContent = signal;
      signals.appendChild(line);
    }

    wrapper.addEventListener("click", (event) => {
      const button = event.target.closest("button[data-action]");
      if (!button) {
        return;
      }

      const action = button.dataset.action;
      if (action === "copy") {
        copyText(state).then(() => {
          button.textContent = "Copied";
          setTimeout(() => { button.textContent = "Copy diagnostics"; }, 900);
        });
      } else if (action === "reload") {
        location.reload();
      } else if (action === "hide") {
        setHiddenUntilReload();
        host.remove();
      }
    });

    shadow.appendChild(wrapper);
    (document.documentElement || document.body).appendChild(host);
  }

  function scan() {
    scanTimer = 0;

    if (hiddenUntilReload()) {
      return;
    }

    const signals = collectSignals();
    if (!signals.length) {
      return;
    }

    detectionCount += 1;
    const state = getDiagnostics(signals);
    saveState(state);
    renderPanel(state);

    if (detectionCount >= MAX_SCANS_AFTER_DETECTION && observer) {
      observer.disconnect();
      observer = null;
    }
  }

  function scheduleScan() {
    if (scanTimer) {
      return;
    }

    scanTimer = window.setTimeout(scan, SCAN_DELAY_MS);
  }

  scheduleScan();

  if (document.documentElement) {
    observer = new MutationObserver(scheduleScan);
    observer.observe(document.documentElement, { childList: true, subtree: true });
  }

  document.addEventListener("DOMContentLoaded", scheduleScan, { once: true });
  window.addEventListener("load", scheduleScan, { once: true });
})();
