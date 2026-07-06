"use strict";

window.SLURG_ENTRIES = [
  { url: "chrome://chrome-urls/", title: "Chrome URL List", category: "Core", status: "documented", note: "Built-in index of supported chrome:// pages for this build." },
  { url: "chrome://version/", title: "Chrome Version", category: "Core", status: "documented", note: "Version, command line, executable path and profile path." },
  { url: "chrome://policy/", title: "Chrome Policies", category: "Core", status: "documented", note: "Enterprise/user policy state." },
  { url: "chrome://components/", title: "Chrome Components", category: "Hardware / Runtime", status: "documented", note: "Installed component updater modules." },
  { url: "chrome://gpu/", title: "GPU Info", category: "Hardware / Runtime", status: "documented", note: "GPU process, ANGLE, Vulkan, D3D, feature status." },
  { url: "chrome://system/", title: "System Info", category: "Hardware / Runtime", status: "documented", note: "Build/system diagnostic dump." },
  { url: "chrome://crashes/", title: "Crash Reporting", category: "Diagnostics", status: "documented", note: "Local crash upload/reporting state." },
  { url: "chrome://histograms/", title: "Histograms", category: "Diagnostics", status: "documented", note: "Runtime counters and metrics." },
  { url: "chrome://download-internals/", title: "Download Internals", category: "Internals", status: "internal", note: "Download subsystem diagnostics; useful for ChatGPT download repair testing." },
  { url: "chrome://extensions-internals/", title: "Extension Internals", category: "Internals", status: "internal", note: "Extension registry/service-worker internals." },
  { url: "chrome://prefs-internals/", title: "Prefs Internals", category: "Internals", status: "internal", note: "Raw preference/debug view. Changes between builds." },
  { url: "chrome://process-internals/#general", title: "Process Model Internals", category: "Internals", status: "internal", note: "Site/process model diagnostics." },
  { url: "chrome://profile-internals/", title: "Profile Internals", category: "Internals", status: "internal", note: "Profile state, useful for taskbar/profile-picker debugging." },
  { url: "chrome://quota-internals/", title: "Quota Internals", category: "Internals", status: "internal", note: "Storage quota diagnostics." },
  { url: "chrome://safe-browsing/", title: "Safe Browsing", category: "Internals", status: "internal", note: "Safe Browsing diagnostics; availability depends on build/services." },
  { url: "chrome://signin-internals/", title: "Sign-In Internals", category: "Internals", status: "internal", note: "Chrome account/sign-in diagnostics." },
  { url: "chrome://accessibility/", title: "Accessibility Internals", category: "Internals", status: "internal", note: "Accessibility tree/state diagnostics." },
  { url: "chrome://autofill-internals/#Context=y&Parsing=y&AbortParsing=y&Filling=y&Submission=y&AutofillServer=y&Metrics=y&AddressProfileFormImport=y&WebsiteModifiedFieldValue=y", title: "Autofill Internals", category: "Internals", status: "internal", note: "Autofill parser/filling diagnostics." },
  { url: "chrome://attribution-internals/", title: "Attribution Internals", category: "Internals", status: "internal", note: "Attribution reporting diagnostics." },
  { url: "chrome://app-service-internals/", title: "App Service Internals", category: "Internals", status: "internal", note: "App service registry/debug state." },
  { url: "chrome://bluetooth-internals/#adapter", title: "Bluetooth Internals", category: "Hardware / Runtime", status: "conditional", note: "Requires Bluetooth stack/support." },
  { url: "chrome://device-log/", title: "Device Log", category: "Hardware / Runtime", status: "conditional", note: "Device event log; contents depend on platform." },
  { url: "chrome://power/", title: "Power / Battery", category: "Hardware / Runtime", status: "conditional", note: "More useful on ChromeOS/laptops; can be unavailable or sparse." },
  { url: "chrome://network/", title: "Network Diagnostics", category: "Networking", status: "conditional", note: "Platform/build dependent network diagnostics page." },
  { url: "chrome://network-errors/", title: "Network Errors", category: "Networking", status: "documented", note: "Network error interstitial reference." },
  { url: "chrome://certificate-manager/", title: "Certificate Manager", category: "Networking", status: "documented", note: "Certificate management UI where supported." },
  { url: "chrome://interstitials/", title: "Interstitials", category: "Networking", status: "internal", note: "Security/error interstitial test pages." },
  { url: "chrome://inspect/#devices", title: "Inspect / Devices", category: "Networking", status: "documented", note: "DevTools remote/device inspection." },
  { url: "chrome://web-app-internals/", title: "Web App Internals", category: "Internals", status: "internal", note: "Installed web apps diagnostics." },
  { url: "chrome://drive-internals/", title: "Drive Internals", category: "ChromeOS / Conditional", status: "conditional", note: "ChromeOS/Drive integration diagnostics." },
  { url: "chrome://family-link-user-internals/", title: "Family Link User Internals", category: "ChromeOS / Conditional", status: "conditional", note: "Family Link state; availability depends on account/platform." },
  { url: "chrome://internet-detail-dialog/", title: "Internet Detail Dialog", category: "ChromeOS / Conditional", status: "conditional", note: "ChromeOS network detail dialog; usually unavailable on desktop Chromium." },
  { url: "chrome://sys-internals/", title: "System Internals", category: "ChromeOS / Conditional", status: "conditional", note: "ChromeOS system internals; desktop builds may not expose it." }
];

window.SLURG_OMITTED = [
  { title: "Bookmarklets that enable/disable extensions", reason: "Extension management bypass actions are intentionally not included." },
  { title: "Password/cookie extractor bookmarklets", reason: "Credential/session extraction is intentionally not included." },
  { title: "Proxy/bypass bookmarklets and exploit links", reason: "Circumvention/exploitation entries are intentionally not included." }
];
