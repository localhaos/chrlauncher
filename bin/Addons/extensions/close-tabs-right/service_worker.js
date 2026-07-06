"use strict";

const MENU_CLOSE_LEFT = "close-tabs-left";
const MENU_CLOSE_RIGHT = "close-tabs-right";

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

chrome.runtime.onInstalled.addListener(setupMenus);
chrome.runtime.onStartup.addListener(setupMenus);
chrome.contextMenus.onClicked.addListener((info, tab) => {
  if (info.menuItemId === MENU_CLOSE_LEFT) {
    closeTabsToLeft(tab);
  } else if (info.menuItemId === MENU_CLOSE_RIGHT) {
    closeTabsToRight(tab);
  }
});
chrome.action.onClicked.addListener(closeTabsToRight);

setupMenus();
