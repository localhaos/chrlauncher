# Extension Guard

Production addon that protects local unpacked chrlauncher addons from disappearing after Google Sync.

Problem it addresses:

- Google account sync may restore extension state from the cloud profile.
- Local unpacked addons are not Chrome Web Store extensions.
- Without stable extension IDs and a protected startup command line, they can appear as removed, disabled, or missing after sync.

Protection layers:

1. Local addon manifests use a fixed `manifest.key`, so their extension IDs stay stable.
2. `Config.ini` uses `--disable-sync-types=Extensions,Apps,ExtensionSettings`, keeping account sync usable while preventing extension/app sync from overriding local addons.
3. `chrome++.ini` is self-healed by native bootstraps and `optimizer_guard`.
4. This addon provides a manual repair script for already-damaged local state.
5. Optional user-policy script can disable only extension/app sync when the Chromium build ignores the command-line sync-type switch.

Manual repair:

```bat
Addons\extension_guard\repair-extensions-after-sync.bat
```

Optional policy-level guard:

```bat
Addons\extension_guard\apply-user-extension-sync-policy.bat
```

Dry run:

```bat
Addons\extension_guard\apply-user-extension-sync-policy.bat -WhatIfOnly
```

Chromium policy only:

```bat
Addons\extension_guard\apply-user-extension-sync-policy.bat -ChromiumOnly
```

After repair:

1. Close all Chromium/Chrome processes.
2. Start `chrlauncher` again.
3. Open `chrome://extensions` and verify:
   - `chrlauncher Tab Tools`
   - `chrlauncher CF Manual Helper`

This does not bypass Chrome Web Store, does not install remote extensions, and does not modify Google account data. The optional policy script writes HKCU browser policy values only when the user runs it explicitly.
