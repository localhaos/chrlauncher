# DevTools Lag Guard

Optional chrlauncher Addon implemented as a standalone Chromium extension.

This addon is intentionally **not loaded by default**. It registers a DevTools panel, and DevTools panels are initialized when F12 opens. On heavy media/social pages such as Facebook, this extra initialization can contribute to renderer or DevTools frontend crashes.

Default production behavior:

```text
Addons\extensions\close-tabs-right
Addons\cf_manual_helper
```

Optional manual loading for diagnostics only:

```text
--load-extension="%app%\..\addons\extensions\close-tabs-right,%app%\..\addons\cf_manual_helper,%app%\..\addons\devtools_lag_guard"
```

The panel stays disabled on these hosts even when manually loaded:

- facebook.com
- fbcdn.net
- messenger.com

Use it only when you are debugging a heavy page and can tolerate changes caused by MutationObserver/timer throttling.
