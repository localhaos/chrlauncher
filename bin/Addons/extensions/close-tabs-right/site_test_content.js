"use strict";

const css = `
.adbox.banner_ads.adsbox,
.textads,
[id*="ad" i],
[class*="banner_ads" i],
[class*="adsbox" i] {
  display: none !important;
  visibility: hidden !important;
  opacity: 0 !important;
  width: 0 !important;
  height: 0 !important;
  min-width: 0 !important;
  min-height: 0 !important;
  pointer-events: none !important;
}
`;

function injectStyle() {
  const style = document.createElement("style");
  style.textContent = css;
  (document.documentElement || document.head || document.body).appendChild(style);
}

function cleanup() {
  const selectors = [
    ".adbox.banner_ads.adsbox",
    ".textads"
  ];

  for (const selector of selectors) {
    for (const node of document.querySelectorAll(selector)) {
      node.remove();
    }
  }
}

injectStyle();

if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", cleanup, { once: true });
} else {
  cleanup();
}

const observer = new MutationObserver(cleanup);
observer.observe(document.documentElement, { childList: true, subtree: true });
