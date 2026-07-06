# CF Manual Helper

Standalone chrlauncher Addon implemented as a small local Chromium extension.

Path:

```text
Addons\cf_manual_helper\
```

Purpose:

- detect common Cloudflare/security challenge pages;
- show a local manual-session diagnostic overlay;
- store the last diagnostics in extension storage;
- let the user copy diagnostics from the page overlay or extension popup.

Explicit non-goals:

- no Turnstile auto-clicking;
- no challenge solving;
- no `cf_clearance` generation;
- no request mirroring;
- no stealth browser or anti-bot bypass.

Loading:

`chrome_plus` receives a comma-separated `--load-extension` value through `chrome++.ini`:

```text
%app%\..\addons\extensions\close-tabs-right,%app%\..\addons\cf_manual_helper
```

The same value is protected by `Addons\optimizer_guard\optimizer_guard.ini` so updates do not remove the addon from the startup command line.
