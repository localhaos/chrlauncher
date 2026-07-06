"use strict";

(() => {
  const MARKER_ID = "chrlauncher-cf-manual-helper";

  function textIncludesAny(value, needles) {
    const haystack = String(value || "").toLowerCase();
    return needles.some((needle) => haystack.includes(needle));
  }

  function hasCloudflareSignals() {
    const title = document.title || "";
    const bodyText = document.body ? document.body.innerText || "" : "";
    const html = document.documentElement ? document.documentElement.innerHTML || "" : "";

    if (textIncludesAny(title, [
      "just a moment",
      "attention required",
      "checking your browser",
      "cloudflare"
    ])) {
      return true;
    }

    if (textIncludesAny(bodyText, [
      "checking if the site connection is secure",
      "verify you are human",
      "needs to review the security of your connection",
      "ray id",
      "performance & security by cloudflare"
    ])) {
      return true;
    }

    if (textIncludesAny(html, [
      "cf-chl-widget",
      "cf-turnstile",
      "cf_clearance",
      "challenge-platform",
      "cloudflare-static"
    ])) {
      return true;
    }

    return false;
  }

  function getCookiePresence() {
    try {
      return document.cookie.split(";").some((entry) => entry.trim().startsWith("cf_clearance="));
    } catch (_) {
      return false;
    }
  }

  function injectPanel() {
    if (document.getElementById(MARKER_ID)) {
      return;
    }

    const panel = document.createElement("div");
    panel.id = MARKER_ID;
    panel.style.cssText = [
      "position:fixed",
      "z-index:2147483647",
      "right:12px",
      "bottom:12px",
      "max-width:360px",
      "font:12px/1.45 system-ui,-apple-system,Segoe UI,sans-serif",
      "background:#111827",
      "color:#f9fafb",
      "border:1px solid #374151",
      "border-radius:10px",
      "box-shadow:0 10px 30px rgba(0,0,0,.35)",
      "padding:12px"
    ].join(";");

    const cookieState = getCookiePresence() ? "cf_clearance: present" : "cf_clearance: not visible to this page script";

    panel.innerHTML = "" +
      "<div style=\"font-weight:700;margin-bottom:6px\">chrlauncher: Cloudflare manual helper</div>" +
      "<div style=\"margin-bottom:8px\">Detected a Cloudflare/security challenge. This helper does not bypass or solve it automatically.</div>" +
      "<div style=\"margin-bottom:8px;color:#d1d5db\">Solve the challenge manually in this normal browser session. Reuse only sessions and cookies you are authorized to use.</div>" +
      "<div style=\"margin-bottom:8px;color:#9ca3af\">" + cookieState + "</div>" +
      "<button type=\"button\" id=\"chrlauncher-cf-helper-close\" style=\"background:#374151;color:#fff;border:0;border-radius:6px;padding:6px 10px;cursor:pointer\">Close</button>";

    (document.body || document.documentElement).appendChild(panel);

    const closeButton = document.getElementById("chrlauncher-cf-helper-close");
    if (closeButton) {
      closeButton.addEventListener("click", () => panel.remove());
    }
  }

  function run() {
    if (hasCloudflareSignals()) {
      injectPanel();
    }
  }

  run();

  const observer = new MutationObserver(() => {
    if (hasCloudflareSignals()) {
      injectPanel();
    }
  });

  if (document.documentElement) {
    observer.observe(document.documentElement, { childList: true, subtree: true });
  }
})();
