// ============================================================
//  Storage Manager — LittleFS Payload CRUD
// ============================================================

#include "storage_manager.h"
#include "config.h"

#include <LittleFS.h>

// ----------------------------------------------------------------
bool storageInit() {
  if (!LittleFS.begin(true)) { // true = format on fail
    Serial.println("[Storage] LittleFS mount failed!");
    return false;
  }

  // Ensure payload directory exists
  if (!LittleFS.exists(PAYLOAD_DIR)) {
    LittleFS.mkdir(PAYLOAD_DIR);
  }

  // Ensure config directory exists
  if (!LittleFS.exists("/config")) {
    LittleFS.mkdir("/config");
  }

  Serial.println("[Storage] LittleFS mounted OK");
  return true;
}

// ----------------------------------------------------------------
std::vector<String> listPayloads() {
  std::vector<String> result;
  File dir = LittleFS.open(PAYLOAD_DIR);
  if (!dir || !dir.isDirectory())
    return result;

  File entry;
  while ((entry = dir.openNextFile())) {
    if (!entry.isDirectory()) {
      result.push_back(entry.name());
    }
    entry.close();
  }
  dir.close();
  return result;
}

// ----------------------------------------------------------------
String readPayload(const String &name) {
  String path = String(PAYLOAD_DIR) + "/" + name;
  File f = LittleFS.open(path, "r");
  if (!f)
    return "";
  String content = f.readString();
  f.close();
  return content;
}

// ----------------------------------------------------------------
bool savePayload(const String &name, const String &content) {
  if (content.length() > MAX_PAYLOAD_SIZE)
    return false;

  String path = String(PAYLOAD_DIR) + "/" + name;
  File f = LittleFS.open(path, "w");
  if (!f)
    return false;
  f.print(content);
  f.close();
  return true;
}

// ----------------------------------------------------------------
bool deletePayload(const String &name) {
  String path = String(PAYLOAD_DIR) + "/" + name;
  return LittleFS.remove(path);
}

// ----------------------------------------------------------------
String getAutoRunPayload() {
  File f = LittleFS.open(AUTORUN_FILE, "r");
  if (!f)
    return "";
  String name = f.readString();
  name.trim();
  f.close();
  return name;
}

// ----------------------------------------------------------------
bool setAutoRunPayload(const String &name) {
  if (name.isEmpty()) {
    // Disable autorun by deleting the config file
    LittleFS.remove(AUTORUN_FILE);
    return true;
  }
  File f = LittleFS.open(AUTORUN_FILE, "w");
  if (!f)
    return false;
  f.print(name);
  f.close();
  return true;
}

// ----------------------------------------------------------------
void getStorageInfo(size_t &totalBytes, size_t &usedBytes) {
  totalBytes = LittleFS.totalBytes();
  usedBytes = LittleFS.usedBytes();
}
