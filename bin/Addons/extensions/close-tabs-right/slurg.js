"use strict";

const entries = Array.isArray(window.SLURG_ENTRIES) ? window.SLURG_ENTRIES : [];
const omitted = Array.isArray(window.SLURG_OMITTED) ? window.SLURG_OMITTED : [];
const cards = document.getElementById("cards");
const template = document.getElementById("cardTemplate");
const search = document.getElementById("search");
const statusFilter = document.getElementById("statusFilter");
const summary = document.getElementById("summary");
const copyAll = document.getElementById("copyAll");
const omittedList = document.getElementById("omittedList");

function statusLabel(status) {
  if (status === "documented") return "documented";
  if (status === "conditional") return "conditional";
  return "internal / undocumented";
}

function visibleEntries() {
  const needle = (search.value || "").trim().toLowerCase();
  const status = statusFilter.value;

  return entries.filter((entry) => {
    if (status !== "all" && entry.status !== status) {
      return false;
    }

    if (!needle) {
      return true;
    }

    return [entry.url, entry.title, entry.category, entry.status, entry.note]
      .join(" ")
      .toLowerCase()
      .includes(needle);
  });
}

function setSummary() {
  const counts = entries.reduce((acc, entry) => {
    acc.total += 1;
    acc[entry.status] = (acc[entry.status] || 0) + 1;
    return acc;
  }, { total: 0 });

  summary.innerHTML = "";

  for (const [label, value] of [
    ["total", counts.total],
    ["documented", counts.documented || 0],
    ["internal", counts.internal || 0],
    ["conditional", counts.conditional || 0]
  ]) {
    const row = document.createElement("div");
    const left = document.createElement("span");
    const right = document.createElement("strong");
    left.textContent = label;
    right.textContent = String(value);
    row.append(left, right);
    summary.append(row);
  }
}

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

function render() {
  const list = visibleEntries();
  cards.innerHTML = "";

  for (const entry of list) {
    const node = template.content.cloneNode(true);
    const card = node.querySelector(".card");
    const badge = node.querySelector(".badge");
    const category = node.querySelector(".category");
    const title = node.querySelector(".title");
    const url = node.querySelector(".url");
    const note = node.querySelector(".note");
    const open = node.querySelector(".open");
    const copy = node.querySelector(".copy");

    badge.textContent = statusLabel(entry.status);
    badge.classList.add(entry.status);
    category.textContent = entry.category;
    title.textContent = entry.title;
    url.textContent = entry.url;
    note.textContent = entry.note;

    open.addEventListener("click", () => {
      chrome.tabs.create({ url: entry.url });
    });

    copy.addEventListener("click", () => {
      copyText(entry.url).then(() => {
        copy.textContent = "Skopiowano";
        setTimeout(() => { copy.textContent = "Kopiuj"; }, 900);
      });
    });

    card.dataset.status = entry.status;
    cards.append(node);
  }
}

function renderOmitted() {
  omittedList.innerHTML = "";

  for (const item of omitted) {
    const li = document.createElement("li");
    const strong = document.createElement("strong");
    strong.textContent = item.title + ": ";
    li.append(strong, document.createTextNode(item.reason));
    omittedList.append(li);
  }
}

search.addEventListener("input", render);
statusFilter.addEventListener("change", render);
copyAll.addEventListener("click", () => {
  const text = visibleEntries().map((entry) => entry.url).join("\n");
  copyText(text).then(() => {
    copyAll.textContent = "Skopiowano";
    setTimeout(() => { copyAll.textContent = "Kopiuj widoczne URL"; }, 1000);
  });
});

setSummary();
renderOmitted();
render();
