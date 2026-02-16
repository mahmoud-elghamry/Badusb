// ============================================================
//  Web Server — REST API + Static File Serving
// ============================================================

#include "web_server.h"
#include "config.h"
#include "ducky_parser.h"
#include "storage_manager.h"
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

// ================================================================
//  Route Handlers
// ================================================================

// GET /api/payloads — list all payloads
static void handleListPayloads(AsyncWebServerRequest *req) {
  auto payloads = listPayloads();
  JsonDocument doc;
  JsonArray arr = doc["payloads"].to<JsonArray>();
  for (auto &name : payloads) {
    arr.add(name);
  }
  sendJson(req, 200, doc);
}

// GET /api/payloads/<name> — get payload content
static void handleGetPayload(AsyncWebServerRequest *req) {
  String name = req->pathArg(0);
  String content = readPayload(name);
  if (content.isEmpty() && !LittleFS.exists(String(PAYLOAD_DIR) + "/" + name)) {
    req->send(404, "application/json", "{\"error\":\"Not found\"}");
    return;
  }
  JsonDocument doc;
  doc["name"] = name;
  doc["content"] = content;
  doc["size"] = content.length();
  sendJson(req, 200, doc);
}

// POST /api/payloads — save payload  { "name": "...", "content": "..." }
static void handleSavePayload(AsyncWebServerRequest *req, uint8_t *data,
                              size_t len, size_t index, size_t total) {
  // Accumulate body
  static String body;
  if (index == 0)
    body = "";
  body += String((char *)data).substring(0, len);

  if (index + len >= total) {
    JsonDocument doc;
    deserializeJson(doc, body);
    String name = doc["name"] | "";
    String content = doc["content"] | "";

    if (name.isEmpty()) {
      req->send(400, "application/json", "{\"error\":\"Name required\"}");
      return;
    }
    if (savePayload(name, content)) {
      req->send(200, "application/json", "{\"status\":\"saved\"}");
    } else {
      req->send(500, "application/json", "{\"error\":\"Save failed\"}");
    }
  }
}

// DELETE /api/payloads/<name>
static void handleDeletePayload(AsyncWebServerRequest *req) {
  String name = req->pathArg(0);
  if (deletePayload(name)) {
    req->send(200, "application/json", "{\"status\":\"deleted\"}");
  } else {
    req->send(404, "application/json", "{\"error\":\"Not found\"}");
  }
}

// POST /api/execute/<name> — execute a stored payload
static void handleExecutePayload(AsyncWebServerRequest *req) {
  String name = req->pathArg(0);
  String path = String(PAYLOAD_DIR) + "/" + name;

  if (!LittleFS.exists(path)) {
    req->send(404, "application/json", "{\"error\":\"Not found\"}");
    return;
  }
  if (duckyIsRunning()) {
    req->send(409, "application/json", "{\"error\":\"Already running\"}");
    return;
  }
  if (duckyExecuteFile(path)) {
    req->send(200, "application/json", "{\"status\":\"executing\"}");
  } else {
    req->send(500, "application/json", "{\"error\":\"Execution failed\"}");
  }
}

// POST /api/execute/live — execute DuckyScript from POST body
static void handleLiveExecute(AsyncWebServerRequest *req, uint8_t *data,
                              size_t len, size_t index, size_t total) {
  static String body;
  if (index == 0)
    body = "";
  body += String((char *)data).substring(0, len);

  if (index + len >= total) {
    JsonDocument doc;
    deserializeJson(doc, body);
    String script = doc["script"] | "";

    if (script.isEmpty()) {
      req->send(400, "application/json", "{\"error\":\"Script required\"}");
      return;
    }
    if (duckyIsRunning()) {
      req->send(409, "application/json", "{\"error\":\"Already running\"}");
      return;
    }
    if (duckyExecute(script)) {
      req->send(200, "application/json", "{\"status\":\"executing\"}");
    } else {
      req->send(500, "application/json", "{\"error\":\"Execution failed\"}");
    }
  }
}

// POST /api/stop — abort running script
static void handleStop(AsyncWebServerRequest *req) {
  if (!duckyIsRunning()) {
    req->send(200, "application/json", "{\"status\":\"idle\"}");
    return;
  }
  duckyStop();
  req->send(200, "application/json", "{\"status\":\"stopping\"}");
}

// GET /api/status — device info
static void handleStatus(AsyncWebServerRequest *req) {
  JsonDocument doc;
  doc["running"] = duckyIsRunning();
  doc["ssid"] = wifiGetSSID();
  doc["ip"] = wifiGetIP();

  size_t total, used;
  getStorageInfo(total, used);
  doc["storage"]["total"] = total;
  doc["storage"]["used"] = used;
  doc["storage"]["free"] = total - used;

  doc["autorun"] = getAutoRunPayload();

  sendJson(req, 200, doc);
}

// POST /api/settings — update settings
static void handleSettings(AsyncWebServerRequest *req, uint8_t *data,
                           size_t len, size_t index, size_t total) {
  static String body;
  if (index == 0)
    body = "";
  body += String((char *)data).substring(0, len);

  if (index + len >= total) {
    JsonDocument doc;
    deserializeJson(doc, body);

    if (doc.containsKey("autorun")) {
      setAutoRunPayload(doc["autorun"] | "");
    }

    req->send(200, "application/json", "{\"status\":\"updated\"}");
  }
}

// ================================================================
//  Server Initialization
// ================================================================

void webServerInit() {
  // --- REST API routes ---
  server.on("/api/payloads", HTTP_GET, handleListPayloads);

  server.on("^\\/api\\/payloads\\/(.+)$", HTTP_GET, handleGetPayload);

  server.on(
      "/api/payloads", HTTP_POST, [](AsyncWebServerRequest *req) {}, nullptr,
      handleSavePayload);

  server.on("^\\/api\\/payloads\\/(.+)$", HTTP_DELETE, handleDeletePayload);

  server.on(
      "^\\/api\\/execute\\/live$", HTTP_POST, [](AsyncWebServerRequest *req) {},
      nullptr, handleLiveExecute);

  server.on("^\\/api\\/execute\\/(.+)$", HTTP_POST, handleExecutePayload);

  server.on("/api/stop", HTTP_POST, handleStop);

  server.on("/api/status", HTTP_GET, handleStatus);

  server.on(
      "/api/settings", HTTP_POST, [](AsyncWebServerRequest *req) {}, nullptr,
      handleSettings);

  // --- CORS headers ---
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods",
                                       "GET, POST, DELETE, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers",
                                       "Content-Type");

  // --- Static files from LittleFS ---
  server.serveStatic("/", LittleFS, "/www/").setDefaultFile("index.html");

  // --- Captive portal: redirect unknown requests to root ---
  server.onNotFound([](AsyncWebServerRequest *req) { req->redirect("/"); });

  server.begin();
  Serial.printf("[Web] Server started on port %d\n", WEB_SERVER_PORT);
}
