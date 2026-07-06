"use strict";

const STATE_KEY = "chrlauncher_cf_manual_helper_last_state";
const stateElement = document.getElementById("state");
const copyButton = document.getElementById("copy");
let currentText = "";

function copyText(text) {
  if (navigator.clipboard && navigator.clipboard.writeText) {
    return navigator.clipboard.writeText(text);
  }

  const textarea = document.createElement("textarea");
  textarea.value = text;
  textarea.style.position = "fixed";
  textarea.style.opacity = "0";
  document.body.appendChild(textarea);
  textarea.select();
  document.execCommand("copy");
  textarea.remove();
  return Promise.resolve();
}

chrome.storage.local.get(STATE_KEY, (result) => {
  const state = result && result[STATE_KEY] ? result[STATE_KEY] : null;
  currentText = state ? JSON.stringify(state, null, 2) : "No challenge diagnostics captured yet.";
  stateElement.textContent = currentText;
});

copyButton.addEventListener("click", () => {
  copyText(currentText).then(() => {
    copyButton.textContent = "Copied";
    setTimeout(() => { copyButton.textContent = "Copy last diagnostics"; }, 900);
  });
});
