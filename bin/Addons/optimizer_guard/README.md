# Optimizer Guard

Standalone chrlauncher Addon that keeps portable/performance optimization from degrading after Chromium or configuration updates.

## What it does

On every `chrlauncher` startup, `src/optimizer_guard_bootstrap.c` reads:

```text
Addons\optimizer_guard\optimizer_guard.ini
```

and re-applies selected settings to:

```text
Config.ini
addons\chrome_plus\chrome++.ini
bin\chrome++.ini
```

This makes the optimization self-healing. If an update or manual extraction replaces part of the configuration, the next launcher startup restores the protected values.

## Protected areas

- portable `Data` profile path;
- portable `Cache` path;
- background-network/domain-reliability reduction;
- text/rendering performance flags;
- scrollable medium-width tabs;
- chrome_plus `data_dir` and `cache_dir`;
- extension loading through `chrome++.ini` when direct `bin\chrome.exe` is started.

## Manual apply

Run:

```bat
Addons\optimizer_guard\apply-optimizer-guard.bat
```

Dry-run:

```bat
Addons\optimizer_guard\apply-optimizer-guard.bat -WhatIfOnly
```

## Disable

Edit `optimizer_guard.ini`:

```ini
[optimizer_guard]
Enabled=false
```

## Notes

The guard intentionally uses conservative optimization. It does not enable experimental GPU/Vulkan/DirectX paths by default because those can regress stability across Chromium updates and different machines.
