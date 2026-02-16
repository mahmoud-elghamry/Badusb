// ============================================================
//  DuckyScript Parser — Non-blocking FreeRTOS-based Interpreter
// ============================================================

#include "ducky_parser.h"
#include "config.h"
#include "keyboard_layout.h"
#include "usb_hid.h"


#include <LittleFS.h>
#include <vector>

// --- Internal state (protected by mutex) ---
static SemaphoreHandle_t sMutex = nullptr;
static TaskHandle_t sTaskHandle = nullptr;
static volatile DuckyStatus sStatus = DuckyStatus::IDLE;
static volatile bool sAbort = false;
static String sScript;
static DuckyCallback sCallback = nullptr;

// --- Forward declarations ---
static void parserTask(void *param);
static void executeLine(const String &line, int defaultDelay, String &lastLine);
static uint8_t resolveKey(const String &keyName);
static uint8_t resolveModifier(const String &modName);
static void reportStatus(int line, int total, DuckyStatus st);

// ================================================================
//  Public API
// ================================================================

void duckyInit() { sMutex = xSemaphoreCreateMutex(); }

bool duckyExecute(const String &script, DuckyCallback cb) {
  if (xSemaphoreTake(sMutex, pdMS_TO_TICKS(100)) != pdTRUE)
    return false;

  if (sStatus == DuckyStatus::RUNNING) {
    xSemaphoreGive(sMutex);
    return false;
  }

  sScript = script;
  sCallback = cb;
  sAbort = false;
  sStatus = DuckyStatus::RUNNING;
  xSemaphoreGive(sMutex);

  // Create (or re-create) the parser task
  if (sTaskHandle != nullptr) {
    vTaskDelete(sTaskHandle);
    sTaskHandle = nullptr;
  }

  xTaskCreatePinnedToCore(parserTask, "DuckyParser", PARSER_TASK_STACK, nullptr,
                          PARSER_TASK_PRIO, &sTaskHandle, PARSER_TASK_CORE);

  return true;
}

bool duckyExecuteFile(const String &filePath, DuckyCallback cb) {
  File f = LittleFS.open(filePath, "r");
  if (!f)
    return false;
  String content = f.readString();
  f.close();
  return duckyExecute(content, cb);
}

void duckyStop() { sAbort = true; }

bool duckyIsRunning() { return sStatus == DuckyStatus::RUNNING; }

DuckyStatus duckyGetStatus() { return sStatus; }

// ================================================================
//  FreeRTOS Task — runs the script line-by-line
// ================================================================

static void parserTask(void *param) {
  // Split script into lines
  std::vector<String> lines;
  int start = 0;
  int idx;
  while ((idx = sScript.indexOf('\n', start)) != -1) {
    lines.push_back(sScript.substring(start, idx));
    start = idx + 1;
  }
  if (start < (int)sScript.length()) {
    lines.push_back(sScript.substring(start));
  }

  int totalLines = lines.size();
  int defaultDelay = DEFAULT_CMD_DELAY;
  String lastLine = "";

  reportStatus(0, totalLines, DuckyStatus::RUNNING);

  for (int i = 0; i < totalLines; i++) {
    // Check abort flag
    if (sAbort) {
      releaseAllKeys();
      sStatus = DuckyStatus::ABORTED;
      reportStatus(i, totalLines, DuckyStatus::ABORTED);
      sTaskHandle = nullptr;
      vTaskDelete(nullptr);
      return;
    }

    String line = lines[i];
    line.trim();

    if (line.length() == 0 || line.startsWith("REM") || line.startsWith("//")) {
      continue; // skip blanks and comments
    }

    // Handle DEFAULT_DELAY / DEFAULTDELAY
    if (line.startsWith("DEFAULT_DELAY ") || line.startsWith("DEFAULTDELAY ")) {
      int spaceIdx = line.indexOf(' ');
      defaultDelay = line.substring(spaceIdx + 1).toInt();
      continue;
    }

    // Handle REPEAT
    if (line.startsWith("REPEAT")) {
      int spaceIdx = line.indexOf(' ');
      int count = (spaceIdx >= 0) ? line.substring(spaceIdx + 1).toInt() : 1;
      if (count < 1)
        count = 1;
      for (int r = 0; r < count && !sAbort; r++) {
        String dummy;
        executeLine(lastLine, 0, dummy);
      }
      reportStatus(i + 1, totalLines, DuckyStatus::RUNNING);
      continue;
    }

    executeLine(line, defaultDelay, lastLine);
    lastLine = line;

    reportStatus(i + 1, totalLines, DuckyStatus::RUNNING);

    // Inter-command delay (non-blocking to other tasks)
    if (defaultDelay > 0) {
      vTaskDelay(pdMS_TO_TICKS(defaultDelay));
    }
  }

  releaseAllKeys();
  sStatus = DuckyStatus::FINISHED;
  reportStatus(totalLines, totalLines, DuckyStatus::FINISHED);
  sTaskHandle = nullptr;
  vTaskDelete(nullptr);
}

// ================================================================
//  Command Execution
// ================================================================

static void executeLine(const String &line, int defaultDelay,
                        String &lastLine) {
  // --- DELAY ---
  if (line.startsWith("DELAY ")) {
    int ms = line.substring(6).toInt();
    vTaskDelay(pdMS_TO_TICKS(ms));
    return;
  }

  // --- STRING ---
  if (line.startsWith("STRING ")) {
    typeString(line.substring(7));
    return;
  }
  if (line.startsWith("STRINGLN ")) {
    typeString(line.substring(9));
    pressKey(KEY_ENTER);
    return;
  }

  // --- MOUSE commands ---
  if (line.startsWith("MOUSE_MOVE ")) {
    String args = line.substring(11);
    int spaceIdx = args.indexOf(' ');
    if (spaceIdx > 0) {
      int8_t dx = (int8_t)args.substring(0, spaceIdx).toInt();
      int8_t dy = (int8_t)args.substring(spaceIdx + 1).toInt();
      mouseMove(dx, dy);
    }
    return;
  }
  if (line.startsWith("MOUSE_CLICK")) {
    String arg = line.substring(11);
    arg.trim();
    if (arg.equalsIgnoreCase("RIGHT"))
      mouseClick(1);
    else if (arg.equalsIgnoreCase("MIDDLE"))
      mouseClick(2);
    else
      mouseClick(0);
    return;
  }
  if (line.startsWith("MOUSE_SCROLL ")) {
    int8_t amount = (int8_t)line.substring(13).toInt();
    mouseScroll(amount);
    return;
  }

  // --- Single keys ---
  if (line == "ENTER" || line == "RETURN") {
    pressKey(KEY_ENTER);
    return;
  }
  if (line == "TAB") {
    pressKey(KEY_TAB);
    return;
  }
  if (line == "ESCAPE" || line == "ESC") {
    pressKey(KEY_ESCAPE);
    return;
  }
  if (line == "SPACE") {
    pressKey(KEY_SPACE);
    return;
  }
  if (line == "BACKSPACE" || line == "BKSP") {
    pressKey(KEY_BACKSPACE);
    return;
  }
  if (line == "DELETE" || line == "DEL") {
    pressKey(KEY_DELETE);
    return;
  }
  if (line == "INSERT") {
    pressKey(KEY_INSERT);
    return;
  }
  if (line == "HOME") {
    pressKey(KEY_HOME);
    return;
  }
  if (line == "END") {
    pressKey(KEY_END);
    return;
  }
  if (line == "PAGEUP") {
    pressKey(KEY_PAGE_UP);
    return;
  }
  if (line == "PAGEDOWN") {
    pressKey(KEY_PAGE_DOWN);
    return;
  }
  if (line == "UP" || line == "UPARROW") {
    pressKey(KEY_UP_ARROW);
    return;
  }
  if (line == "DOWN" || line == "DOWNARROW") {
    pressKey(KEY_DOWN_ARROW);
    return;
  }
  if (line == "LEFT" || line == "LEFTARROW") {
    pressKey(KEY_LEFT_ARROW);
    return;
  }
  if (line == "RIGHT" || line == "RIGHTARROW") {
    pressKey(KEY_RIGHT_ARROW);
    return;
  }
  if (line == "CAPSLOCK") {
    pressKey(KEY_CAPSLOCK);
    return;
  }
  if (line == "PRINTSCREEN") {
    pressKey(KEY_PRINT_SCREEN);
    return;
  }
  if (line == "SCROLLLOCK") {
    pressKey(KEY_SCROLL_LOCK);
    return;
  }
  if (line == "PAUSE" || line == "BREAK") {
    pressKey(KEY_PAUSE);
    return;
  }
  if (line == "NUMLOCK") {
    pressKey(KEY_NUM_LOCK);
    return;
  }
  if (line == "MENU" || line == "APP") {
    pressKey(KEY_MENU);
    return;
  }

  // --- Function keys ---
  for (int f = 1; f <= 12; f++) {
    if (line == "F" + String(f)) {
      pressKey(KEY_F1 + f - 1);
      return;
    }
  }

  // --- Modifier combos: GUI/WINDOWS, CTRL, ALT, SHIFT + key ---
  // Supports:  GUI r   |   CTRL ALT DELETE   |   SHIFT TAB   etc.
  uint8_t modMask = 0;
  String remaining = line;

  // Parse modifier tokens from the front
  while (remaining.length() > 0) {
    String token;
    int spaceIdx = remaining.indexOf(' ');
    if (spaceIdx >= 0) {
      token = remaining.substring(0, spaceIdx);
      remaining = remaining.substring(spaceIdx + 1);
      remaining.trim();
    } else {
      token = remaining;
      remaining = "";
    }

    uint8_t mod = resolveModifier(token);
    if (mod != MOD_NONE) {
      modMask |= mod;
    } else {
      // This token is the final key
      uint8_t key = resolveKey(token);
      if (key != KEY_NONE) {
        pressKey(key, modMask);
      } else if (token.length() == 1) {
        // Single character — type it with modifiers
        KeyMapping km = getKeyMapping(token.charAt(0));
        pressKey(km.keycode, modMask | km.modifier);
      }
      return;
    }
  }

  // If we only got modifiers with no final key (e.g. "GUI" alone)
  if (modMask != 0) {
    pressKey(KEY_NONE, modMask);
  }
}

// ================================================================
//  Key & Modifier Resolution (DuckyScript names → HID codes)
// ================================================================

static uint8_t resolveKey(const String &keyName) {
  if (keyName == "ENTER" || keyName == "RETURN")
    return KEY_ENTER;
  if (keyName == "TAB")
    return KEY_TAB;
  if (keyName == "ESCAPE" || keyName == "ESC")
    return KEY_ESCAPE;
  if (keyName == "SPACE")
    return KEY_SPACE;
  if (keyName == "BACKSPACE" || keyName == "BKSP")
    return KEY_BACKSPACE;
  if (keyName == "DELETE" || keyName == "DEL")
    return KEY_DELETE;
  if (keyName == "INSERT")
    return KEY_INSERT;
  if (keyName == "HOME")
    return KEY_HOME;
  if (keyName == "END")
    return KEY_END;
  if (keyName == "PAGEUP")
    return KEY_PAGE_UP;
  if (keyName == "PAGEDOWN")
    return KEY_PAGE_DOWN;
  if (keyName == "UP" || keyName == "UPARROW")
    return KEY_UP_ARROW;
  if (keyName == "DOWN" || keyName == "DOWNARROW")
    return KEY_DOWN_ARROW;
  if (keyName == "LEFT" || keyName == "LEFTARROW")
    return KEY_LEFT_ARROW;
  if (keyName == "RIGHT" || keyName == "RIGHTARROW")
    return KEY_RIGHT_ARROW;
  if (keyName == "CAPSLOCK")
    return KEY_CAPSLOCK;
  if (keyName == "PRINTSCREEN")
    return KEY_PRINT_SCREEN;
  if (keyName == "SCROLLLOCK")
    return KEY_SCROLL_LOCK;
  if (keyName == "PAUSE" || keyName == "BREAK")
    return KEY_PAUSE;
  if (keyName == "NUMLOCK")
    return KEY_NUM_LOCK;
  if (keyName == "MENU" || keyName == "APP")
    return KEY_MENU;

  // Function keys
  for (int f = 1; f <= 12; f++) {
    if (keyName == "F" + String(f))
      return KEY_F1 + f - 1;
  }

  // Single letter/digit
  if (keyName.length() == 1) {
    KeyMapping km = getKeyMapping(keyName.charAt(0));
    return km.keycode;
  }

  return KEY_NONE;
}

static uint8_t resolveModifier(const String &modName) {
  if (modName == "GUI" || modName == "WINDOWS" || modName == "SUPER" ||
      modName == "META")
    return MOD_LEFT_GUI;
  if (modName == "CTRL" || modName == "CONTROL")
    return MOD_LEFT_CTRL;
  if (modName == "ALT")
    return MOD_LEFT_ALT;
  if (modName == "SHIFT")
    return MOD_LEFT_SHIFT;
  return MOD_NONE;
}

// ================================================================
//  Status Reporting
// ================================================================

static void reportStatus(int line, int total, DuckyStatus st) {
  if (sCallback) {
    sCallback(line, total, st);
  }
}
