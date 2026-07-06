"use strict";

const MENU_CLOSE_LEFT = "close-tabs-left";
const MENU_CLOSE_RIGHT = "close-tabs-right";
const MENU_OPEN_SLURG = "open-slurg-diagnostics";
const CHATGPT_STATE_KEY = "chatgpt_download_fix_state";

function openSlurgDiagnostics() {
  chrome.tabs.create({ url: chrome.runtime.getURL("slurg.html") });
}

function setupMenus() {
  chrome.contextMenus.removeAll(() => {
    chrome.contextMenus.create({
      id: MENU_CLOSE_LEFT,
      title: "Zamknij karty po lewej",
      contexts: ["tab", "action"]
    });

    chrome.contextMenus.create({
      id: MENU_CLOSE_RIGHT,
      title: "Zamknij karty po prawej",
      contexts: ["tab", "action"]
    });

    chrome.contextMenus.create({
      id: MENU_OPEN_SLURG,
      title: "Slurg: chrome:// diagnostics",
      contexts: ["action"]
    });
  });
}

function getActiveTab(callback) {
  chrome.tabs.query({ active: true, lastFocusedWindow: true }, (tabs) => {
    callback(tabs && tabs.length ? tabs[0] : null);
  });
}

function closeTabsBefore(anchorTab) {
  if (!anchorTab || anchorTab.id === undefined || anchorTab.windowId === undefined || anchorTab.index === undefined) {
    return;
  }

  chrome.tabs.query({ windowId: anchorTab.windowId }, (tabs) => {
    const tabIds = tabs
      .filter((tab) => !tab.pinned && tab.id !== undefined && tab.index < anchorTab.index)
      .map((tab) => tab.id);

    if (tabIds.length) {
      chrome.tabs.remove(tabIds);
    }
  });
}

function closeTabsAfter(anchorTab) {
  if (!anchorTab || anchorTab.id === undefined || anchorTab.windowId === undefined || anchorTab.index === undefined) {
    return;
  }

  chrome.tabs.query({ windowId: anchorTab.windowId }, (tabs) => {
    const tabIds = tabs
      .filter((tab) => !tab.pinned && tab.id !== undefined && tab.index > anchorTab.index)
      .map((tab) => tab.id);

    if (tabIds.length) {
      chrome.tabs.remove(tabIds);
    }
  });
}

function closeTabsToLeft(tab) {
  if (tab && tab.id !== undefined) {
    closeTabsBefore(tab);
    return;
  }

  getActiveTab(closeTabsBefore);
}

function closeTabsToRight(tab) {
  if (tab && tab.id !== undefined) {
    closeTabsAfter(tab);
    return;
  }

  getActiveTab(closeTabsAfter);
}

function safeFilename(name) {
  const cleaned = String(name || "chatgpt-download")
    .replace(/[<>:"/\\|?*\x00-\x1F]/g, "_")
    .trim()
    .slice(0, 180);

  return cleaned || "chatgpt-download";
}

function filenameFromRequestUrl(requestUrl) {
  try {
    const parsed = new URL(requestUrl);
    const sandboxPath = parsed.searchParams.get("sandbox_path") || "";
    const piece = sandboxPath.split("/").filter(Boolean).pop();
    return piece ? safeFilename(decodeURIComponent(piece)) : "";
  } catch (_) {
    return "";
  }
}

function filenameFromDirectUrl(directUrl) {
  try {
    const parsed = new URL(directUrl);
    const piece = parsed.pathname.split("/").filter(Boolean).pop();
    return piece ? safeFilename(decodeURIComponent(piece)) : "";
  } catch (_) {
    return "";
  }
}

function storageSet(value) {
  chrome.storage.local.set(value, () => {});
}

function rememberChatGptCapture(capture) {
  storageSet({
    [CHATGPT_STATE_KEY]: {
      lastCapture: capture,
      updatedAt: new Date().toISOString()
    }
  });
}

function startChatGptDownload(capture, sendResponse) {
  if (!capture || !capture.directUrl || !/^https?:\/\//i.test(capture.directUrl)) {
    sendResponse({ ok: false, error: "Missing direct download URL." });
    return;
  }

  const filename = safeFilename(
    capture.filename ||
    filenameFromRequestUrl(capture.requestUrl || "") ||
    filenameFromDirectUrl(capture.directUrl || "") ||
    "chatgpt-download"
  );

  const normalized = {
    requestUrl: capture.requestUrl || "",
    directUrl: capture.directUrl,
    filename
  };

  rememberChatGptCapture(normalized);

  chrome.downloads.download({
    url: normalized.directUrl,
    filename,
    saveAs: false,
    conflictAction: "uniquify"
  }, (downloadId) => {
    const err = chrome.runtime.lastError ? String(chrome.runtime.lastError.message || chrome.runtime.lastError) : "";
    if (err) {
      sendResponse({ ok: false, error: err, filename });
      return;
    }

    sendResponse({ ok: true, downloadId, filename });
  });
}

chrome.runtime.onInstalled.addListener(setupMenus);
chrome.runtime.onStartup.addListener(setupMenus);
chrome.contextMenus.onClicked.addListener((info, tab) => {
  if (info.menuItemId === MENU_CLOSE_LEFT) {
    closeTabsToLeft(tab);
  } else if (info.menuItemId === MENU_CLOSE_RIGHT) {
    closeTabsToRight(tab);
  } else if (info.menuItemId === MENU_OPEN_SLURG) {
    openSlurgDiagnostics();
  }
});
chrome.action.onClicked.addListener(closeTabsToRight);

if (chrome.omnibox) {
  chrome.omnibox.onInputEntered.addListener(() => {
    openSlurgDiagnostics();
  });
}

chrome.runtime.onMessage.addListener((message, _sender, sendResponse) => {
  if (!message || typeof message !== "object") {
    sendResponse({ ok: false, error: "Invalid message." });
    return false;
  }

  if (message.type === "CHATGPT_DIRECT_DOWNLOAD") {
    startChatGptDownload({
      requestUrl: message.requestUrl || "",
      directUrl: message.directUrl || "",
      filename: message.filename || ""
    }, sendResponse);
    return true;
  }

  if (message.type === "CHATGPT_DOWNLOAD_FIX_STATE") {
    chrome.storage.local.get(CHATGPT_STATE_KEY, (state) => {
      sendResponse(state && state[CHATGPT_STATE_KEY] ? state[CHATGPT_STATE_KEY] : {});
    });
    return true;
  }

  if (message.type === "OPEN_SLURG") {
    openSlurgDiagnostics();
    sendResponse({ ok: true });
    return false;
  }

  return false;
});

setupMenus();
