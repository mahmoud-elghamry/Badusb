// ============================================================
//  Web Server - REST API and static file serving
// ============================================================

#include "web_server.h"
#include "config.h"
#include "ducky_parser.h"
#include "storage_manager.h"
#include "usb_hid.h"
#include "wifi_manager.h"

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

static AsyncWebServer server(WEB_SERVER_PORT);

// ================================================================
//  Helpers
// ================================================================

static void sendJson(AsyncWebServerRequest *req, int code,
                     const JsonDocument &doc) {
  String body;
  serializeJson(doc, body);
  req->send(code, "application/json", body);
}

static void sendError(AsyncWebServerRequest *req, int code,
                      const char *message) {
  JsonDocument doc;
  doc["error"] = message;
  sendJson(req, code, doc);
}

static const char *statusToString(DuckyStatus status) {
  switch (status) {
  case DuckyStatus::IDLE:
    return "idle";
  case DuckyStatus::RUNNING:
    return "running";
  case DuckyStatus::PAUSED:
    return "paused";
  case DuckyStatus::FINISHED:
    return "finished";
  case DuckyStatus::ERROR:
    return "error";
  case DuckyStatus::ABORTED:
    return "aborted";
  default:
    return "unknown";
  }
}

static bool requestIsAuthorized(AsyncWebServerRequest *req) {
  if (!isAdminTokenConfigured() || !req->hasHeader("X-Admin-Token")) {
    return false;
  }

  const AsyncWebHeader *header = req->getHeader("X-Admin-Token");
  return header && validateAdminToken(header->value());
}

static bool ensureAuthorized(AsyncWebServerRequest *req) {
  if (requestIsAuthorized(req)) {
    return true;
  }
  sendError(req, 401, "Admin token required");
  return false;
}

static bool collectBody(AsyncWebServerRequest *req, uint8_t *data, size_t len,
                        size_t index, size_t total, size_t maxLen,
                        String &body) {
  if (index == 0) {
    if (total > maxLen) {
      sendError(req, 413, "Request body too large");
      return false;
    }

    req->_tempObject = calloc(total + 1, 1);
    if (!req->_tempObject) {
      sendError(req, 500, "Out of memory");
      return false;
    }
  }

  if (!req->_tempObject) {
    return false;
  }

  memcpy(static_cast<char *>(req->_tempObject) + index, data, len);

  if (index + len < total) {
    return false;
  }

  body = String(static_cast<char *>(req->_tempObject));
  free(req->_tempObject);
  req->_tempObject = nullptr;
  return true;
}

static bool readJsonBody(AsyncWebServerRequest *req, uint8_t *data, size_t len,
                         size_t index, size_t total, size_t maxLen,
                         JsonDocument &doc) {
  String body;
  if (!collectBody(req, data, len, index, total, maxLen, body)) {
    return false;
  }

  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    sendError(req, 400, "Invalid JSON");
    return false;
  }
  return true;
}

static bool parseHex16(const String &input, uint16_t &out) {
  String clean = input;
  clean.trim();
  if (clean.length() != 4) {
    return false;
  }

  for (size_t i = 0; i < clean.length(); i++) {
    char c = clean.charAt(i);
    bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F');
    if (!ok) {
      return false;
    }
  }

  out = (uint16_t)strtoul(clean.c_str(), nullptr, 16);
  return out != 0x0000 && out != 0xFFFF;
}

static String payloadPath(const String &name) {
  return String(PAYLOAD_DIR) + "/" + name;
}

// ================================================================
//  Route Handlers
// ================================================================

static void handleSetup(AsyncWebServerRequest *req, uint8_t *data, size_t len,
                        size_t index, size_t total) {
  if (isAdminTokenConfigured()) {
    sendError(req, 409, "Admin token already configured");
    return;
  }

  JsonDocument doc;
  if (!readJsonBody(req, data, len, index, total, MAX_SETUP_BODY_SIZE, doc)) {
    return;
  }

  String token = doc["token"] | "";
  if (!setAdminToken(token)) {
    sendError(req, 400, "Token must be at least 10 characters");
    return;
  }

  JsonDocument res;
  res["status"] = "configured";
  sendJson(req, 200, res);
}

static void handleListPayloads(AsyncWebServerRequest *req) {
  if (!ensureAuthorized(req)) {
    return;
  }

  auto payloads = listPayloads();
  JsonDocument doc;
  JsonArray arr = doc["payloads"].to<JsonArray>();
  for (auto &name : payloads) {
    arr.add(name);
  }
  sendJson(req, 200, doc);
}

static void handleGetPayload(AsyncWebServerRequest *req) {
  if (!ensureAuthorized(req)) {
    return;
  }

  String name = req->pathArg(0);
  if (!payloadExists(name)) {
    sendError(req, 404, "Payload not found");
    return;
  }

  String content = readPayload(name);
  JsonDocument doc;
  doc["name"] = name;
  doc["content"] = content;
  doc["size"] = content.length();
  sendJson(req, 200, doc);
}

static void handleSavePayload(AsyncWebServerRequest *req, uint8_t *data,
                              size_t len, size_t index, size_t total) {
  if (!requestIsAuthorized(req)) {
    if (index == 0) {
      sendError(req, 401, "Admin token required");
    }
    return;
  }

  JsonDocument doc;
  if (!readJsonBody(req, data, len, index, total, MAX_API_BODY_SIZE, doc)) {
    return;
  }

  String name = doc["name"] | "";
  String content = doc["content"] | "";
  if (!isValidPayloadName(name)) {
    sendError(req, 400, "Invalid payload name");
    return;
  }
  if (content.length() > MAX_PAYLOAD_SIZE) {
    sendError(req, 413, "Payload too large");
    return;
  }

  if (!savePayload(name, content)) {
    sendError(req, 500, "Save failed");
    return;
  }

  JsonDocument res;
  res["status"] = "saved";
  sendJson(req, 200, res);
}

static void handleDeletePayload(AsyncWebServerRequest *req) {
  if (!ensureAuthorized(req)) {
    return;
  }

  String name = req->pathArg(0);
  if (!payloadExists(name)) {
    sendError(req, 404, "Payload not found");
    return;
  }

  if (getAutoRunPayload() == name) {
    setAutoRunPayload("");
  }

  if (deletePayload(name)) {
    JsonDocument doc;
    doc["status"] = "deleted";
    sendJson(req, 200, doc);
  } else {
    sendError(req, 500, "Delete failed");
  }
}

static bool ensureExecutionAllowed(AsyncWebServerRequest *req) {
  if (!ensureAuthorized(req)) {
    return false;
  }
  if (!usbIsReady()) {
    sendError(req, 409, "USB HID is not active in this boot mode");
    return false;
  }
  if (duckyIsRunning()) {
    sendError(req, 409, "A script is already running");
    return false;
  }
  return true;
}

static void handleExecutePayload(AsyncWebServerRequest *req) {
  if (!ensureExecutionAllowed(req)) {
    return;
  }

  String name = req->pathArg(0);
  if (!payloadExists(name)) {
    sendError(req, 404, "Payload not found");
    return;
  }

  if (!duckyExecuteFile(payloadPath(name))) {
    sendError(req, 500, "Execution failed");
    return;
  }

  JsonDocument doc;
  doc["status"] = "executing";
  sendJson(req, 200, doc);
}

static void handleLiveExecute(AsyncWebServerRequest *req, uint8_t *data,
                              size_t len, size_t index, size_t total) {
  if (!requestIsAuthorized(req)) {
    if (index == 0) {
      sendError(req, 401, "Admin token required");
    }
    return;
  }
  if (!usbIsReady()) {
    if (index == 0) {
      sendError(req, 409, "USB HID is not active in this boot mode");
    }
    return;
  }
  if (duckyIsRunning()) {
    if (index == 0) {
      sendError(req, 409, "A script is already running");
    }
    return;
  }

  JsonDocument doc;
  if (!readJsonBody(req, data, len, index, total, MAX_LIVE_SCRIPT_SIZE + 64,
                    doc)) {
    return;
  }

  String script = doc["script"] | "";
  if (script.isEmpty()) {
    sendError(req, 400, "Script required");
    return;
  }
  if (script.length() > MAX_LIVE_SCRIPT_SIZE) {
    sendError(req, 413, "Live script too large");
    return;
  }

  if (!duckyExecute(script)) {
    sendError(req, 500, "Execution failed");
    return;
  }

  JsonDocument res;
  res["status"] = "executing";
  sendJson(req, 200, res);
}

static void handleStop(AsyncWebServerRequest *req) {
  if (!ensureAuthorized(req)) {
    return;
  }

  if (!duckyIsRunning()) {
    JsonDocument doc;
    doc["status"] = "idle";
    sendJson(req, 200, doc);
    return;
  }

  duckyStop();
  JsonDocument doc;
  doc["status"] = "stopping";
  sendJson(req, 200, doc);
}

static void handleStatus(AsyncWebServerRequest *req) {
  bool authorized = requestIsAuthorized(req);

  JsonDocument doc;
  doc["setupRequired"] = !isAdminTokenConfigured();
  doc["authenticated"] = authorized;
  doc["running"] = duckyIsRunning();
  doc["duckyStatus"] = statusToString(duckyGetStatus());
  doc["lastError"] = duckyGetLastError();
  doc["hidReady"] = usbIsReady();
  doc["executionEnabled"] = authorized && usbIsReady();
  doc["ssid"] = wifiGetSSID();
  doc["ip"] = wifiGetIP();

  if (authorized) {
    size_t total, used;
    getStorageInfo(total, used);
    doc["storage"]["total"] = total;
    doc["storage"]["used"] = used;
    doc["storage"]["free"] = total > used ? total - used : 0;
    doc["autorun"] = getAutoRunPayload();
  }

  sendJson(req, 200, doc);
}

static void handleSettings(AsyncWebServerRequest *req, uint8_t *data,
                           size_t len, size_t index, size_t total) {
  if (!requestIsAuthorized(req)) {
    if (index == 0) {
      sendError(req, 401, "Admin token required");
    }
    return;
  }

  JsonDocument doc;
  if (!readJsonBody(req, data, len, index, total, MAX_SETTINGS_BODY_SIZE, doc)) {
    return;
  }

  if (doc["autorun"].is<const char *>()) {
    String name = doc["autorun"] | "";
    if (!setAutoRunPayload(name)) {
      sendError(req, 400, "Invalid autorun payload");
      return;
    }
  }

  if (doc["vid"].is<const char *>() || doc["pid"].is<const char *>()) {
    String vidStr = doc["vid"] | "";
    String pidStr = doc["pid"] | "";
    uint16_t vid = 0;
    uint16_t pid = 0;
    if (!parseHex16(vidStr, vid) || !parseHex16(pidStr, pid)) {
      sendError(req, 400, "VID and PID must be 4 hex digits, not 0000/FFFF");
      return;
    }
    if (!setUsbIdentity(vid, pid)) {
      sendError(req, 500, "Failed to save USB identity");
      return;
    }
  }

  JsonDocument res;
  res["status"] = "updated";
  sendJson(req, 200, res);
}

static void handleGetSettings(AsyncWebServerRequest *req) {
  if (!ensureAuthorized(req)) {
    return;
  }

  JsonDocument doc;
  uint16_t vid, pid;
  getUsbIdentity(vid, pid);

  char vidHex[5], pidHex[5];
  snprintf(vidHex, sizeof(vidHex), "%04X", vid);
  snprintf(pidHex, sizeof(pidHex), "%04X", pid);

  doc["vid"] = String(vidHex);
  doc["pid"] = String(pidHex);
  doc["autorun"] = getAutoRunPayload();
  sendJson(req, 200, doc);
}

// ================================================================
//  Server Initialization
// ================================================================

void webServerInit() {
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods",
                                       "GET, POST, DELETE, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers",
                                       "Content-Type, X-Admin-Token");

  server.on("^\\/api\\/.*$", HTTP_OPTIONS,
            [](AsyncWebServerRequest *req) { req->send(204); });

  server.on("/api/status", HTTP_GET, handleStatus);

  server.on("/api/setup", HTTP_POST, [](AsyncWebServerRequest *req) {}, nullptr,
            handleSetup);

  server.on("/api/payloads", HTTP_GET, handleListPayloads);
  server.on("^\\/api\\/payloads\\/(.+)$", HTTP_GET, handleGetPayload);
  server.on("/api/payloads", HTTP_POST, [](AsyncWebServerRequest *req) {},
            nullptr, handleSavePayload);
  server.on("^\\/api\\/payloads\\/(.+)$", HTTP_DELETE, handleDeletePayload);

  server.on("^\\/api\\/execute\\/live$", HTTP_POST,
            [](AsyncWebServerRequest *req) {}, nullptr, handleLiveExecute);
  server.on("^\\/api\\/execute\\/(.+)$", HTTP_POST, handleExecutePayload);
  server.on("/api/stop", HTTP_POST, handleStop);

  server.on("/api/settings", HTTP_GET, handleGetSettings);
  server.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest *req) {},
            nullptr, handleSettings);

  server.serveStatic("/", LittleFS, "/www/").setDefaultFile("index.html");

  server.onNotFound([](AsyncWebServerRequest *req) {
    if (req->url().startsWith("/api/")) {
      sendError(req, 404, "API endpoint not found");
    } else {
      req->redirect("/");
    }
  });

  server.begin();
  Serial.printf("[Web] Server started on port %d\n", WEB_SERVER_PORT);
}
