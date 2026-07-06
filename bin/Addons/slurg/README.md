# Slurg Diagnostics

Standalone chrlauncher Addon for safe Chrome/Chromium diagnostics.

This is intentionally not a Chrome extension. It is a local HTML addon that can be opened directly from:

```text
Addons\slurg\index.html
```

The addon lists safe diagnostic `chrome://` pages and classifies them as:

- `documented`
- `internal / undocumented`
- `conditional / ChromeOS`

The source list was derived only from safe diagnostic Chrome URL sections. Exploit, bypass, proxy, password extractor, cookie extractor and extension-management bookmarklets are intentionally omitted.

Limitations:

- A real new `chrome://slurg` WebUI page cannot be registered by a local addon or extension. That requires patching Chromium WebUI registration.
- Opening `chrome://` targets from a local `file://` page can be restricted by Chromium in some builds. Use copy buttons if direct links are blocked.
