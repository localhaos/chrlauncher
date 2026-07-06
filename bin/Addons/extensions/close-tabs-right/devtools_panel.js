"use strict";

const output = document.getElementById("output");

function formatResult(result, isException) {
  if (isException) {
    return "EVAL ERROR\n" + JSON.stringify(isException, null, 2);
  }

  if (typeof result === "string") {
    return result;
  }

  return JSON.stringify(result, null, 2);
}

function ensureState() {
  window.__chrlauncherDevToolsLagGuard = window.__chrlauncherDevToolsLagGuard || {
    installedAt: new Date().toISOString(),
    styleId: "chrlauncher-devtools-lag-guard-style",
    originals: {},
    mutationObservers: [],
    observersPaused: false,
    timersThrottled: false
  };

  return window.__chrlauncherDevToolsLagGuard;
}

function runInInspectedPage(fn, args = []) {
  return new Promise((resolve) => {
    const source = `(() => {\n${ensureState.toString()}\nreturn (${fn.toString()}).apply(null, ${JSON.stringify(args)});\n})()`;
    chrome.devtools.inspectedWindow.eval(source, { useContentScriptContext: false }, (result, isException) => {
      resolve(formatResult(result, isException));
    });
  });
}

function scanDom() {
  const all = Array.from(document.getElementsByTagName("*"));
  const totalElements = all.length;
  let maxChildren = { tag: "", id: "", className: "", children: 0 };
  let fixedOrSticky = 0;
  let animated = 0;

  for (const element of all) {
    if (element.children && element.children.length > maxChildren.children) {
      maxChildren = {
        tag: element.tagName.toLowerCase(),
        id: element.id || "",
        className: typeof element.className === "string" ? element.className.slice(0, 120) : "",
        children: element.children.length
      };
    }

    try {
      const cs = getComputedStyle(element);
      if (cs.position === "fixed" || cs.position === "sticky") fixedOrSticky++;
      if (cs.animationName !== "none" || cs.transitionDuration !== "0s") animated++;
    } catch (_) {}
  }

  const observerState = window.__chrlauncherDevToolsLagGuard || null;

  return {
    url: location.href,
    title: document.title,
    totalElements,
    fixedOrSticky,
    animatedOrTransitioning: animated,
    largestDirectChildContainer: maxChildren,
    lagGuard: observerState ? {
      observersWrapped: observerState.mutationObservers ? observerState.mutationObservers.length : 0,
      observersPaused: Boolean(observerState.observersPaused),
      timersThrottled: Boolean(observerState.timersThrottled),
      liteCss: Boolean(document.getElementById(observerState.styleId))
    } : "not installed yet",
    notes: [
      totalElements > 50000 ? "Very large DOM: DevTools Elements panel can freeze." : "DOM size is moderate or unknown.",
      animated > 500 ? "Many animated/transitioning elements: try Lite CSS." : "Animation count is not extreme.",
      "For rapidly mutating pages, use MutationObserver/timer controls only while debugging."
    ]
  };
}

function applyLiteCss() {
  const state = ensureState();
  let style = document.getElementById(state.styleId);

  if (!style) {
    style = document.createElement("style");
    style.id = state.styleId;
    style.textContent = `
      *, *::before, *::after {
        animation: none !important;
        transition: none !important;
        scroll-behavior: auto !important;
      }
      html, body {
        scroll-behavior: auto !important;
      }
      [style*="filter"], [style*="backdrop-filter"] {
        filter: none !important;
        backdrop-filter: none !important;
      }
      video, canvas {
        image-rendering: auto !important;
      }
    `;
    (document.head || document.documentElement).appendChild(style);
  }

  return {
    ok: true,
    action: "lite-css",
    message: "Animations, transitions, smooth scrolling and heavy filter effects disabled for the inspected page."
  };
}

function pauseMutationObservers() {
  const state = ensureState();

  if (!state.originals.MutationObserver && window.MutationObserver) {
    state.originals.MutationObserver = window.MutationObserver;
    const NativeMutationObserver = window.MutationObserver;

    window.MutationObserver = function (callback) {
      const observer = new NativeMutationObserver(callback);
      const nativeObserve = observer.observe.bind(observer);
      const nativeDisconnect = observer.disconnect.bind(observer);
      const record = { observer, nativeObserve, nativeDisconnect, targets: [] };

      observer.observe = function (target, options) {
        record.targets.push({ target, options });
        if (state.observersPaused) {
          return undefined;
        }
        return nativeObserve(target, options);
      };

      observer.disconnect = function () {
        return nativeDisconnect();
      };

      state.mutationObservers.push(record);
      return observer;
    };

    window.MutationObserver.prototype = NativeMutationObserver.prototype;
  }

  state.observersPaused = true;
  for (const record of state.mutationObservers) {
    try { record.nativeDisconnect(); } catch (_) {}
  }

  return {
    ok: true,
    action: "pause-observers",
    trackedObservers: state.mutationObservers.length,
    message: "Tracked MutationObservers disconnected. New observers created after this hook will not observe until restore."
  };
}

function throttleTimers() {
  const state = ensureState();

  if (!state.originals.requestAnimationFrame && typeof window.requestAnimationFrame === "function") {
    state.originals.requestAnimationFrame = window.requestAnimationFrame.bind(window);
    window.requestAnimationFrame = function (callback) {
      return window.setTimeout(() => callback(performance.now()), 250);
    };
  }

  if (!state.originals.setInterval && typeof window.setInterval === "function") {
    state.originals.setInterval = window.setInterval.bind(window);
    window.setInterval = function (callback, delay, ...args) {
      const safeDelay = Math.max(Number(delay) || 0, 250);
      return state.originals.setInterval(callback, safeDelay, ...args);
    };
  }

  state.timersThrottled = true;

  return {
    ok: true,
    action: "throttle-timers",
    message: "requestAnimationFrame is slowed and setInterval minimum delay is 250 ms. Restore when finished debugging."
  };
}

function restorePage() {
  const state = ensureState();
  const style = document.getElementById(state.styleId);

  if (style) style.remove();

  if (state.originals.MutationObserver) {
    window.MutationObserver = state.originals.MutationObserver;
  }

  if (state.originals.requestAnimationFrame) {
    window.requestAnimationFrame = state.originals.requestAnimationFrame;
  }

  if (state.originals.setInterval) {
    window.setInterval = state.originals.setInterval;
  }

  state.observersPaused = false;
  state.timersThrottled = false;

  return {
    ok: true,
    action: "restore",
    message: "Lite CSS removed and patched APIs restored where possible. Existing page state may require reload for full reset."
  };
}

const actions = {
  "stats": scanDom,
  "lite-css": applyLiteCss,
  "pause-observers": pauseMutationObservers,
  "throttle-timers": throttleTimers,
  "restore": restorePage
};

document.addEventListener("click", async (event) => {
  const button = event.target.closest("button[data-action]");
  if (!button) return;

  const action = button.dataset.action;
  const fn = actions[action];
  if (!fn) return;

  output.textContent = "Wykonuję: " + action + " ...";
  const result = await runInInspectedPage(fn);
  output.textContent = result;
});
