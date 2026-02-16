// ============================================================
//  Wi-Fi Manager — Access Point + Captive Portal
// ============================================================

#include "wifi_manager.h"
#include "config.h"

#include <DNSServer.h>
#include <WiFi.h>


static DNSServer sDns;
static String sSSID;

void wifiInit() {
  // Build SSID from MAC suffix
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%02X%02X", mac[4], mac[5]);
  sSSID = String(WIFI_SSID_PREFIX) + suffix;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(sSSID.c_str(), WIFI_PASSWORD, WIFI_CHANNEL, 0, WIFI_MAX_CLIENTS);

  // Start DNS server for captive portal — redirect all domains to us
  sDns.start(53, "*", WiFi.softAPIP());

  Serial.printf("[WiFi] AP started — SSID: %s  IP: %s\n", sSSID.c_str(),
                WiFi.softAPIP().toString().c_str());
}

String wifiGetSSID() { return sSSID; }

String wifiGetIP() { return WiFi.softAPIP().toString(); }
