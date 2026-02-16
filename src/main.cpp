// ============================================================
//  BadUSB ESP32-S3 — Main Entry Point
// ============================================================
//
//  Boot Safety Logic:
//    1. LED blinks for SAFETY_WINDOW_MS (2 seconds)
//    2. If BOOT button (GPIO0) is held → Config Mode (Wi-Fi only)
//    3. If BOOT button is released    → Attack Mode (execute payload)
//
// ============================================================

#include "config.h"
#include "ducky_parser.h"
#include "storage_manager.h"
#include "usb_hid.h"
#include "web_server.h"
#include "wifi_manager.h"
#include <Arduino.h>


// --- Boot mode ---
enum BootMode { MODE_ATTACK, MODE_CONFIG };

static BootMode detectBootMode();
static void blinkLED(int count, int intervalMs);

// ================================================================
//  Setup
// ================================================================

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n=== BadUSB ESP32-S3 ===");

  // LED & button pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

  // Initialize storage first (needed by all modes)
  if (!storageInit()) {
    Serial.println("[FATAL] Storage init failed — halting.");
    while (true) {
      blinkLED(3, 100);
      delay(500);
    }
  }

  // Initialize DuckyScript parser (creates FreeRTOS task infrastructure)
  duckyInit();

  // Detect boot mode (2-second safety window)
  BootMode mode = detectBootMode();

  if (mode == MODE_CONFIG) {
    // --- CONFIG MODE ---
    Serial.println("[Boot] CONFIG MODE — Wi-Fi + Web UI only");
    digitalWrite(LED_PIN, HIGH); // solid LED = config mode

    wifiInit();
    webServerInit();

    Serial.printf("[Boot] Connect to Wi-Fi: %s  Password: %s\n",
                  wifiGetSSID().c_str(), WIFI_PASSWORD);
    Serial.printf("[Boot] Open http://%s in your browser\n",
                  wifiGetIP().c_str());

  } else {
    // --- ATTACK MODE ---
    Serial.println("[Boot] ATTACK MODE — Initializing USB HID");

    // Initialize USB HID (keyboard + mouse)
    initUSB();

    // Wait for host OS to enumerate
    delay(1500);

    // Fix keyboard layout to English
    fixLayout();
    delay(200);

    // Check for autorun payload
    String autorun = getAutoRunPayload();
    if (autorun.length() > 0) {
      String path = String(PAYLOAD_DIR) + "/" + autorun;
      Serial.printf("[Boot] Auto-running payload: %s\n", autorun.c_str());

      duckyExecuteFile(path, [](int line, int total, DuckyStatus st) {
        if (st == DuckyStatus::FINISHED) {
          Serial.println("[Ducky] Payload execution finished.");
        } else if (st == DuckyStatus::ERROR) {
          Serial.println("[Ducky] Payload execution error!");
        } else if (st == DuckyStatus::ABORTED) {
          Serial.println("[Ducky] Payload execution aborted.");
        }
      });
    } else {
      Serial.println("[Boot] No autorun payload configured.");
    }

    // Also start Wi-Fi in attack mode (background, for remote control)
    wifiInit();
    webServerInit();
    Serial.printf("[Boot] Wi-Fi active in background: %s\n",
                  wifiGetSSID().c_str());
  }

  Serial.println("[Boot] Setup complete.\n");
}

// ================================================================
//  Loop
// ================================================================

void loop() {
  // FreeRTOS handles the parser task.
  // Wi-Fi + Web server run on the other core.
  // Nothing needed in loop — yield to scheduler.
  vTaskDelay(pdMS_TO_TICKS(100));
}

// ================================================================
//  Boot Mode Detection (2-second safety window)
// ================================================================

static BootMode detectBootMode() {
  Serial.println("[Boot] Safety window — hold BOOT button for Config Mode...");

  unsigned long start = millis();
  bool buttonHeld = true;

  while (millis() - start < SAFETY_WINDOW_MS) {
    // Blink LED to indicate safety window
    digitalWrite(LED_PIN, (millis() / SAFETY_BLINK_MS) % 2 ? HIGH : LOW);

    // BOOT button is active LOW
    if (digitalRead(BOOT_BUTTON_PIN) == HIGH) {
      buttonHeld = false;
    }

    delay(10);
  }

  digitalWrite(LED_PIN, LOW);

  // If button was held the entire window → Config Mode
  if (buttonHeld && digitalRead(BOOT_BUTTON_PIN) == LOW) {
    return MODE_CONFIG;
  }

  return MODE_ATTACK;
}

// ================================================================
//  LED Utility
// ================================================================

static void blinkLED(int count, int intervalMs) {
  for (int i = 0; i < count; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(intervalMs);
    digitalWrite(LED_PIN, LOW);
    delay(intervalMs);
  }
}
