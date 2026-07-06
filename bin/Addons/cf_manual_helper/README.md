# cf_manual_helper

Minimalist safe helper for Cloudflare-protected pages.

This addon intentionally does not bypass Cloudflare, does not click Turnstile, does not generate `cf_clearance`, does not mirror requests, and does not evade anti-bot controls.

What it does:

- detects common Cloudflare challenge/interstitial signals in a normal user browser session;
- shows a local diagnostic overlay;
- reminds the user to solve the challenge manually and only reuse sessions they are authorized to use.

Why it exists:

The referenced `sarperavci/CloudflareBypassForScraping` project describes cookie generation, request mirroring, stealth browser use, and challenge solving. Those are not included here. This repository keeps a compliant manual-session helper only.

Implementation location:

- loaded through `bin/Addons/extensions/close-tabs-right/manifest.json`;
- script: `bin/Addons/extensions/close-tabs-right/cf_manual_helper.js`.
