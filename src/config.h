#pragma once

// ============================================================
//  ESP32-S3 HID Lab - Global Configuration
// ============================================================

// --- USB Identity ---
// Defaults can be overridden from the web UI and stored in LittleFS.
#define DEFAULT_USB_VID 0x1234
#define DEFAULT_USB_PID 0x5678
#define USB_MANUFACTURER "Generic"
#define USB_PRODUCT "USB Keyboard"

// --- Wi-Fi Access Point ---
#define WIFI_SSID_PREFIX "HIDLab_"
#define WIFI_PASSWORD "change-this-pass"
#define WIFI_CHANNEL 1
#define WIFI_MAX_CLIENTS 2

// --- Web Server ---
#define WEB_SERVER_PORT 80

// --- Storage ---
#define PAYLOAD_DIR "/payloads"
#define AUTORUN_FILE "/config/autorun.txt"
#define MAX_PAYLOAD_SIZE (64 * 1024)
#define MAX_PAYLOAD_NAME_LEN 48
#define FORMAT_FS_ON_FAIL false

// --- Web/API Safety ---
// On first use, set an admin token from the web UI. Mutating and sensitive API
// routes reject requests without X-Admin-Token after setup.
#define ADMIN_TOKEN_HASH_FILE "/config/admin_hash.txt"
#define MIN_ADMIN_TOKEN_LEN 10
#define MAX_API_BODY_SIZE (MAX_PAYLOAD_SIZE + 1024)
#define MAX_LIVE_SCRIPT_SIZE (8 * 1024)
#define MAX_SETTINGS_BODY_SIZE 512
#define MAX_SETUP_BODY_SIZE 256

// --- Boot Safety ---
#define BOOT_BUTTON_PIN 0
#define SAFETY_WINDOW_MS 2000
#define SAFETY_BLINK_MS 200

// --- Status LED ---
#define LED_PIN 2

// --- DuckyScript Parser ---
#define DEFAULT_CMD_DELAY 0
#define MAX_COMMAND_DELAY_MS 600000
#define PARSER_TASK_STACK 8192
#define PARSER_TASK_PRIO 1
#define PARSER_TASK_CORE 0

// --- Keyboard Layout Fix ---
#define FIX_LAYOUT_DELAY 100
