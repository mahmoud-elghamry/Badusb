#pragma once

// ============================================================
//  Wi-Fi Manager — Access Point + Captive Portal
// ============================================================

#include <Arduino.h>

/// Start the ESP32 as a Wi-Fi Access Point.
/// SSID is auto-generated as "HIDLab_XXXX" (last 4 hex of MAC).
void wifiInit();

/// Process captive portal DNS requests. Call from loop().
void wifiProcess();

/// Get the full SSID string.
String wifiGetSSID();

/// Get the AP IP address string.
String wifiGetIP();
