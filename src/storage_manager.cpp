// ============================================================
//  Storage Manager - LittleFS payload and configuration storage
// ============================================================

#include "storage_manager.h"
#include "config.h"

#include <LittleFS.h>
#include <algorithm>
#include <mbedtls/sha256.h>

static const char *USB_ID_FILE = "/config/usb_id.txt";

static bool ensureDirectory(const char *path) {
  if (LittleFS.exists(path)) {
    File dir = LittleFS.open(path, "r");
    bool ok = dir && dir.isDirectory();
    dir.close();
    return ok;
  }
  return LittleFS.mkdir(path);
}

static String basenameOf(const String &path) {
  int slash = path.lastIndexOf('/');
  int backslash = path.lastIndexOf('\\');
  int pos = max(slash, backslash);
  return pos >= 0 ? path.substring(pos + 1) : path;
}

static String payloadPath(const String &name) {
  return String(PAYLOAD_DIR) + "/" + name;
}

static bool isValidHexId(uint16_t value) {
  return value != 0x0000 && value != 0xFFFF;
}

static String sha256Hex(const String &value) {
  unsigned char hash[32];
  char hex[65];

  mbedtls_sha256_ret(reinterpret_cast<const unsigned char *>(value.c_str()),
                     value.length(), hash, 0);

  for (size_t i = 0; i < sizeof(hash); i++) {
    snprintf(hex + (i * 2), 3, "%02x", hash[i]);
  }
  hex[64] = '\0';
  return String(hex);
}

// ----------------------------------------------------------------
bool storageInit() {
  if (!LittleFS.begin(FORMAT_FS_ON_FAIL)) {
    Serial.println("[Storage] LittleFS mount failed.");
    return false;
  }

  if (!ensureDirectory(PAYLOAD_DIR) || !ensureDirectory("/config")) {
    Serial.println("[Storage] Required directories are missing.");
    return false;
  }

  Serial.println("[Storage] LittleFS mounted OK");
  return true;
}

// ----------------------------------------------------------------
bool isValidPayloadName(const String &name) {
  if (name.isEmpty() || name.length() > MAX_PAYLOAD_NAME_LEN) {
    return false;
  }
  if (name == "." || name == ".." || name.startsWith(".") ||
      name.indexOf("..") >= 0 || name.indexOf('/') >= 0 ||
      name.indexOf('\\') >= 0) {
    return false;
  }

  for (size_t i = 0; i < name.length(); i++) {
    char c = name.charAt(i);
    bool allowed = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                   (c >= '0' && c <= '9') || c == '_' || c == '-' ||
                   c == '.';
    if (!allowed) {
      return false;
    }
  }
  return true;
}

// ----------------------------------------------------------------
bool payloadExists(const String &name) {
  return isValidPayloadName(name) && LittleFS.exists(payloadPath(name));
}

// ----------------------------------------------------------------
std::vector<String> listPayloads() {
  std::vector<String> result;
  File dir = LittleFS.open(PAYLOAD_DIR, "r");
  if (!dir || !dir.isDirectory()) {
    return result;
  }

  File entry;
  while ((entry = dir.openNextFile())) {
    if (!entry.isDirectory()) {
      String name = basenameOf(entry.name());
      if (isValidPayloadName(name)) {
        result.push_back(name);
      }
    }
    entry.close();
  }
  dir.close();

  std::sort(result.begin(), result.end(),
            [](const String &a, const String &b) { return a < b; });
  return result;
}

// ----------------------------------------------------------------
String readPayload(const String &name) {
  if (!isValidPayloadName(name)) {
    return "";
  }

  File f = LittleFS.open(payloadPath(name), "r");
  if (!f) {
    return "";
  }

  String content = f.readString();
  f.close();
  return content;
}

// ----------------------------------------------------------------
bool savePayload(const String &name, const String &content) {
  if (!isValidPayloadName(name) || content.length() > MAX_PAYLOAD_SIZE) {
    return false;
  }

  File f = LittleFS.open(payloadPath(name), "w");
  if (!f) {
    return false;
  }

  size_t written = f.print(content);
  f.close();
  return written == content.length();
}

// ----------------------------------------------------------------
bool deletePayload(const String &name) {
  if (!isValidPayloadName(name)) {
    return false;
  }
  return LittleFS.remove(payloadPath(name));
}

// ----------------------------------------------------------------
String getAutoRunPayload() {
  File f = LittleFS.open(AUTORUN_FILE, "r");
  if (!f) {
    return "";
  }

  String name = f.readString();
  name.trim();
  f.close();

  if (!payloadExists(name)) {
    return "";
  }
  return name;
}

// ----------------------------------------------------------------
bool setAutoRunPayload(const String &name) {
  if (name.isEmpty()) {
    LittleFS.remove(AUTORUN_FILE);
    return true;
  }
  if (!payloadExists(name)) {
    return false;
  }

  File f = LittleFS.open(AUTORUN_FILE, "w");
  if (!f) {
    return false;
  }

  size_t written = f.print(name);
  f.close();
  return written == name.length();
}

// ----------------------------------------------------------------
void getStorageInfo(size_t &totalBytes, size_t &usedBytes) {
  totalBytes = LittleFS.totalBytes();
  usedBytes = LittleFS.usedBytes();
}

// ----------------------------------------------------------------
void getUsbIdentity(uint16_t &vid, uint16_t &pid) {
  vid = DEFAULT_USB_VID;
  pid = DEFAULT_USB_PID;

  File f = LittleFS.open(USB_ID_FILE, "r");
  if (!f) {
    return;
  }

  String content = f.readString();
  content.trim();
  f.close();

  int comma = content.indexOf(',');
  if (comma <= 0) {
    return;
  }

  char *vidEnd = nullptr;
  char *pidEnd = nullptr;
  unsigned long parsedVid =
      strtoul(content.substring(0, comma).c_str(), &vidEnd, 16);
  unsigned long parsedPid =
      strtoul(content.substring(comma + 1).c_str(), &pidEnd, 16);

  if (parsedVid <= 0xFFFF && parsedPid <= 0xFFFF &&
      isValidHexId((uint16_t)parsedVid) && isValidHexId((uint16_t)parsedPid)) {
    vid = (uint16_t)parsedVid;
    pid = (uint16_t)parsedPid;
  }
}

// ----------------------------------------------------------------
bool setUsbIdentity(uint16_t vid, uint16_t pid) {
  if (!isValidHexId(vid) || !isValidHexId(pid)) {
    return false;
  }

  File f = LittleFS.open(USB_ID_FILE, "w");
  if (!f) {
    return false;
  }

  int written = f.printf("%04X,%04X", vid, pid);
  f.close();
  return written == 9;
}

// ----------------------------------------------------------------
bool isAdminTokenConfigured() {
  return LittleFS.exists(ADMIN_TOKEN_HASH_FILE);
}

// ----------------------------------------------------------------
bool setAdminToken(const String &token) {
  String clean = token;
  clean.trim();
  if (clean.length() < MIN_ADMIN_TOKEN_LEN || clean.length() > 128) {
    return false;
  }

  File f = LittleFS.open(ADMIN_TOKEN_HASH_FILE, "w");
  if (!f) {
    return false;
  }

  String hash = sha256Hex(clean);
  size_t written = f.print(hash);
  f.close();
  return written == hash.length();
}

// ----------------------------------------------------------------
bool validateAdminToken(const String &token) {
  String clean = token;
  clean.trim();
  if (clean.length() < MIN_ADMIN_TOKEN_LEN || !isAdminTokenConfigured()) {
    return false;
  }

  File f = LittleFS.open(ADMIN_TOKEN_HASH_FILE, "r");
  if (!f) {
    return false;
  }

  String expected = f.readString();
  expected.trim();
  f.close();

  return expected.length() == 64 && expected == sha256Hex(clean);
}
