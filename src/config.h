#pragma once

// ============================================================
//  BadUSB ESP32-S3 — Global Configuration
// ============================================================

// --- USB Identity (spoof as a generic keyboard) ---
#define USB_VID           0x1234
#define USB_PID           0x5678
#define USB_MANUFACTURER  "Generic"
#define USB_PRODUCT       "USB Keyboard"

// --- Wi-Fi Access Point ---
#define WIFI_SSID_PREFIX  "BadUSB_"
#define WIFI_PASSWORD     "badusb1234"
#define WIFI_CHANNEL      1
#define WIFI_MAX_CLIENTS  2

// --- Web Server ---
#define WEB_SERVER_PORT   80

// --- Storage ---
#define PAYLOAD_DIR       "/payloads"
#define AUTORUN_FILE      "/config/autorun.txt"   // stores name of auto-run payload
#define MAX_PAYLOAD_SIZE  (64 * 1024)             // 64 KB max per script

// --- Boot Safety ---
#define BOOT_BUTTON_PIN   0       // GPIO0 = BOOT button on most dev boards
#define SAFETY_WINDOW_MS  2000    // hold BOOT for 2 s → Config Mode
#define SAFETY_BLINK_MS   200     // LED blink rate during safety window

// --- Status LED ---
#define LED_PIN           2       // on-board LED (adjust for your board)

// --- DuckyScript Parser ---
#define DEFAULT_CMD_DELAY 0       // ms between commands (overridden by DEFAULT_DELAY)
#define PARSER_TASK_STACK 8192    // FreeRTOS task stack size (bytes)
#define PARSER_TASK_PRIO  1       // FreeRTOS task priority
#define PARSER_TASK_CORE  0       // pin to core 0 (core 1 for Wi-Fi)

// --- Keyboard Layout Fix ---
#define FIX_LAYOUT_DELAY  100     // ms to hold ALT+SHIFT for layout switch
