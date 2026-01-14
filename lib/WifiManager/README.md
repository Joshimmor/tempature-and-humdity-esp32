/**
 * @file wifimanager.h
 * @brief WiFiManager — a high-level WiFi management module for ESP32.
 *
 * This module centralizes WiFi-related functionality and state for ESP32-based
 * applications, providing a simple, robust API for:
 *  - Initializing the WiFi stack and related subsystems (event loop, NVS).
 *  - Connecting to saved or provided STA networks with automatic reconnection.
 *  - Starting an Access Point (AP) for provisioning or local services.
 *  - Scanning for nearby SSIDs and reporting results.
 *  - Managing credential storage (save/erase) in non-volatile storage (NVS).
 *  - Registering callbacks for connection, disconnection, scan results, and errors.
 *
 * Key design goals:
 *  - Minimal application-side complexity: expose high-level operations.
 *  - Safe automated reconnect with backoff and optional entropy/randomization.
 *  - Non-blocking API where feasible; offer synchronous helpers for simple flows.
 *  - Small footprint suitable for constrained ESP32 applications.
 *
 * Features:
 *  - Automatic reconnection and link monitoring.
 *  - Optional fallback to AP mode when STA connect fails (provisioning mode).
 *  - Credential persistence and optional factory reset behavior.
 *  - Scan filtering and results callback.
 *  - Event-driven callbacks compatible with FreeRTOS tasks.
 *
 * Typical usage:
 *  1. Call wifimanager_init() early in startup (before network ops).
 *  2. Optionally load credentials from NVS and call wifimanager_connect().
 *  3. Register event callbacks with wifimanager_register_callback() to react
 *     to CONNECTED/DISCONNECTED/SCAN_DONE events.
 *  4. Use wifimanager_start_ap() to enable AP/provisioning mode if needed.
 *
 * Public API (high-level overview):
 *  - wifimanager_init(const wifimanager_config_t *cfg)
 *      Initialize module; must be called once. Configures reconnection policy,
 *      NVS keys, AP defaults, and logging level.
 *
 *  - wifimanager_deinit(void)
 *      Deinitialize module and release resources.
 *
 *  - wifimanager_connect(const char *ssid, const char *pass, wifimanager_conn_opts_t *opts)
 *      Attempt to connect to the specified SSID. If ssid==NULL, attempt to
 *      connect using stored credentials. Returns immediately; connection progress
 *      reported via callbacks or status query.
 *
 *  - wifimanager_disconnect(bool persist)
 *      Disconnect from current STA. If persist==false, clear any transient state
 *      but do not erase saved credentials.
 *
 *  - wifimanager_start_ap(const char *ap_ssid, const char *ap_pass, int channel, int max_conn)
 *      Start the module in AP mode for provisioning or local network access.
 *
 *  - wifimanager_stop_ap(void)
 *      Stop AP mode and restore prior STA behavior.
 *
 *  - wifimanager_scan(const wifimanager_scan_params_t *params)
 *      Initiate an asynchronous WiFi scan; results delivered through callback.
 *
 *  - wifimanager_save_credentials(const char *ssid, const char *pass)
 *      Persist credentials to NVS (encrypted if configured).
 *
 *  - wifimanager_erase_credentials(void)
 *      Remove stored credentials from NVS (factory-reset behavior).
 *
 *  - wifimanager_get_status(wifimanager_status_t *status_out)
 *      Query current connection/AP status and last error codes.
 *
 *  - wifimanager_register_callback(wifimanager_event_cb_t cb, void *ctx)
 *      Register application callback for high-level WiFi events.
 *
 * Events / Callbacks:
 *  - WIFI_EVENT_CONNECTED: SSID/BSSID, IP address available.
 *  - WIFI_EVENT_DISCONNECTED: reason code; reconnection attempts may follow.
 *  - WIFI_EVENT_GOT_IP: DHCP success (if applicable).
 *  - WIFI_EVENT_SCAN_DONE: scan results available; use provided iterator or copy.
 *  - WIFI_EVENT_AP_STARTED / WIFI_EVENT_AP_STOPPED: AP lifecycle events.
 *  - WIFI_EVENT_ERROR: persistent error such as NVS failure, internal error.
 *
 * Error handling:
 *  - Functions return error codes (wifimanager_err_t) for immediate failures.
 *  - Asynchronous failures are reported via WIFI_EVENT_ERROR with an error code.
 *  - Common errors: WM_ERR_OK, WM_ERR_INVALID_ARG, WM_ERR_NO_MEM, WM_ERR_NVS,
 *    WM_ERR_NOT_INITIALIZED, WM_ERR_NO_CREDENTIALS, WM_ERR_CONNECT_FAILED.
 *
 * Threading and reentrancy:
 *  - Public API functions are safe to call from application tasks.
 *  - Callbacks may be invoked from the WiFi event task context; keep callbacks
 *    short and use FreeRTOS synchronization primitives or dispatch to your task
 *    for heavy work.
 *
 * Security and privacy:
 *  - Credentials stored in NVS should be protected; the module supports optional
 *    encryption/obfuscation via a compile-time or runtime key if required.
 *  - Avoid logging plaintext passwords; the module's logging masks sensitive data.
 *
 * Configuration notes:
 *  - Provide a wifimanager_config_t at init to control:
 *      - Reconnect policy (attempts, backoff strategy).
 *      - Default AP SSID/pass and visibility.
 *      - NVS namespace/keys for credential storage.
 *      - DHCP/static IP settings for STA/AP if needed.
 *
 * Example (pseudo):
 *  {
 *      wifimanager_config_t cfg = { .auto_reconnect = true, .ap_fallback = true, ... };
 *      wifimanager_init(&cfg);
 *      wifimanager_register_callback(my_wifi_cb, NULL);
 *      if (!wifimanager_connect(NULL, NULL, NULL)) {
 *          // No saved credentials — start provisioning AP
 *          wifimanager_start_ap("esp32-setup", "setup1234", 6, 4);
 *      }
 *  }
 *
 * Notes and limitations:
 *  - Module assumes underlying ESP-IDF (or compatible) WiFi driver; adapt if
 *    using alternate SDKs or low-level drivers.
 *  - For low-latency, high-throughput applications, tune power management and
 *    WiFi config flags outside this module as needed.
 *
 * Authors:
 *  - (Module maintainer / project author)
 *
 * License:
 *  - (Insert project license here)
 */