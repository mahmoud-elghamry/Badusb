// ============================================================
//  DuckyScript Parser - FreeRTOS-based interpreter
// ============================================================

#include "ducky_parser.h"
#include "config.h"
#include "keyboard_layout.h"
#include "usb_hid.h"

#include <LittleFS.h>

static SemaphoreHandle_t sMutex = nullptr;
static TaskHandle_t sTaskHandle = nullptr;
static volatile DuckyStatus sStatus = DuckyStatus::IDLE;
static volatile bool sAbort = false;
static String sScript;
static String sLastError;
static DuckyCallback sCallback = nullptr;

static void parserTask(void *param);
static bool executeLine(const String &line, String &error);
static uint8_t resolveKey(const String &keyName);
static uint8_t resolveModifier(const String &modName);
static void reportStatus(int line, int total, DuckyStatus st);

static void setStatus(DuckyStatus status) { sStatus = status; }

static void setLastError(const String &error) {
  if (sMutex && xSemaphoreTake(sMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    sLastError = error;
    xSemaphoreGive(sMutex);
  } else {
    sLastError = error;
  }
}

static bool parseLong(const String &input, long &value) {
  String clean = input;
  clean.trim();
  if (clean.isEmpty()) {
    return false;
  }

  char *end = nullptr;
  value = strtol(clean.c_str(), &end, 10);
  return end && *end == '\0';
}

static bool parseLongInRange(const String &input, long minValue, long maxValue,
                             long &value) {
  if (!parseLong(input, value)) {
    return false;
  }
  return value >= minValue && value <= maxValue;
}

static bool waitWithAbort(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    if (sAbort) {
      return false;
    }
    unsigned long elapsed = millis() - start;
    unsigned long remaining = ms - elapsed;
    unsigned long slice = remaining > 25 ? 25 : remaining;
    vTaskDelay(pdMS_TO_TICKS(slice));
  }
  return true;
}

static int countLines(const String &script) {
  if (script.isEmpty()) {
    return 0;
  }

  int lines = 1;
  for (size_t i = 0; i < script.length(); i++) {
    if (script.charAt(i) == '\n') {
      lines++;
    }
  }
  return lines;
}

static bool nextLine(const String &script, size_t &cursor, String &line) {
  if (cursor >= script.length()) {
    return false;
  }

  int next = script.indexOf('\n', cursor);
  if (next < 0) {
    line = script.substring(cursor);
    cursor = script.length();
  } else {
    line = script.substring(cursor, next);
    cursor = next + 1;
  }
  line.trim();
  return true;
}

static bool isCommentOrBlank(const String &line) {
  if (line.isEmpty() || line.startsWith("//")) {
    return true;
  }

  String upper = line;
  upper.toUpperCase();
  return upper == "REM" || upper.startsWith("REM ");
}

// ================================================================
//  Public API
// ================================================================

void duckyInit() {
  if (!sMutex) {
    sMutex = xSemaphoreCreateMutex();
  }
}

bool duckyExecute(const String &script, DuckyCallback cb) {
  if (!sMutex) {
    duckyInit();
  }
  if (!sMutex || script.length() > MAX_PAYLOAD_SIZE) {
    return false;
  }

  if (xSemaphoreTake(sMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    return false;
  }

  if (sStatus == DuckyStatus::RUNNING) {
    xSemaphoreGive(sMutex);
    return false;
  }

  sScript = script;
  sCallback = cb;
  sLastError = "";
  sAbort = false;
  sStatus = DuckyStatus::RUNNING;
  xSemaphoreGive(sMutex);

  BaseType_t created = xTaskCreatePinnedToCore(
      parserTask, "DuckyParser", PARSER_TASK_STACK, nullptr, PARSER_TASK_PRIO,
      &sTaskHandle, PARSER_TASK_CORE);

  if (created != pdPASS) {
    setLastError("Failed to create parser task");
    setStatus(DuckyStatus::ERROR);
    sTaskHandle = nullptr;
    return false;
  }

  return true;
}

bool duckyExecuteFile(const String &filePath, DuckyCallback cb) {
  File f = LittleFS.open(filePath, "r");
  if (!f) {
    setLastError("Payload file could not be opened");
    return false;
  }
  if (f.size() > MAX_PAYLOAD_SIZE) {
    f.close();
    setLastError("Payload file is too large");
    return false;
  }

  String content = f.readString();
  f.close();
  return duckyExecute(content, cb);
}

void duckyStop() { sAbort = true; }

bool duckyIsRunning() { return sStatus == DuckyStatus::RUNNING; }

DuckyStatus duckyGetStatus() { return sStatus; }

String duckyGetLastError() {
  if (sMutex && xSemaphoreTake(sMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    String error = sLastError;
    xSemaphoreGive(sMutex);
    return error;
  }
  return sLastError;
}

// ================================================================
//  FreeRTOS Task
// ================================================================

static void parserTask(void *param) {
  String script;
  if (xSemaphoreTake(sMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    script = sScript;
    xSemaphoreGive(sMutex);
  } else {
    setLastError("Parser could not lock script state");
    setStatus(DuckyStatus::ERROR);
    sTaskHandle = nullptr;
    vTaskDelete(nullptr);
    return;
  }

  int totalLines = countLines(script);
  int currentLine = 0;
  long defaultDelay = DEFAULT_CMD_DELAY;
  String lastExecutableLine;
  size_t cursor = 0;

  reportStatus(0, totalLines, DuckyStatus::RUNNING);

  String line;
  while (nextLine(script, cursor, line)) {
    currentLine++;
    if (sAbort) {
      releaseAllKeys();
      setStatus(DuckyStatus::ABORTED);
      reportStatus(currentLine, totalLines, DuckyStatus::ABORTED);
      sTaskHandle = nullptr;
      vTaskDelete(nullptr);
      return;
    }

    if (isCommentOrBlank(line)) {
      continue;
    }

    String upper = line;
    upper.toUpperCase();

    if (upper.startsWith("DEFAULT_DELAY ") ||
        upper.startsWith("DEFAULTDELAY ")) {
      int spaceIdx = line.indexOf(' ');
      long parsedDelay = 0;
      if (!parseLongInRange(line.substring(spaceIdx + 1), 0,
                            MAX_COMMAND_DELAY_MS, parsedDelay)) {
        setLastError("Invalid DEFAULT_DELAY at line " + String(currentLine));
        setStatus(DuckyStatus::ERROR);
        reportStatus(currentLine, totalLines, DuckyStatus::ERROR);
        sTaskHandle = nullptr;
        vTaskDelete(nullptr);
        return;
      }
      defaultDelay = parsedDelay;
      continue;
    }

    if (upper == "REPEAT" || upper.startsWith("REPEAT ")) {
      long count = 0;
      String countArg = line.length() > 6 ? line.substring(6) : "1";
      if (!parseLongInRange(countArg, 1, 1000, count)) {
        setLastError("Invalid REPEAT count at line " + String(currentLine));
        setStatus(DuckyStatus::ERROR);
        reportStatus(currentLine, totalLines, DuckyStatus::ERROR);
        sTaskHandle = nullptr;
        vTaskDelete(nullptr);
        return;
      }
      if (lastExecutableLine.isEmpty()) {
        setLastError("REPEAT has no previous command at line " +
                     String(currentLine));
        setStatus(DuckyStatus::ERROR);
        reportStatus(currentLine, totalLines, DuckyStatus::ERROR);
        sTaskHandle = nullptr;
        vTaskDelete(nullptr);
        return;
      }

      for (long r = 0; r < count && !sAbort; r++) {
        String error;
        if (!executeLine(lastExecutableLine, error)) {
          setLastError(error + " at line " + String(currentLine));
          setStatus(DuckyStatus::ERROR);
          reportStatus(currentLine, totalLines, DuckyStatus::ERROR);
          sTaskHandle = nullptr;
          vTaskDelete(nullptr);
          return;
        }
        if (defaultDelay > 0 && !waitWithAbort(defaultDelay)) {
          break;
        }
      }
      reportStatus(currentLine, totalLines, DuckyStatus::RUNNING);
      continue;
    }

    String error;
    if (!executeLine(line, error)) {
      releaseAllKeys();
      setLastError(error + " at line " + String(currentLine));
      setStatus(DuckyStatus::ERROR);
      reportStatus(currentLine, totalLines, DuckyStatus::ERROR);
      sTaskHandle = nullptr;
      vTaskDelete(nullptr);
      return;
    }

    lastExecutableLine = line;
    reportStatus(currentLine, totalLines, DuckyStatus::RUNNING);

    if (defaultDelay > 0 && !waitWithAbort(defaultDelay)) {
      break;
    }
  }

  releaseAllKeys();
  setStatus(sAbort ? DuckyStatus::ABORTED : DuckyStatus::FINISHED);
  reportStatus(totalLines, totalLines, duckyGetStatus());
  sTaskHandle = nullptr;
  vTaskDelete(nullptr);
}

// ================================================================
//  Command Execution
// ================================================================

static bool executeLine(const String &line, String &error) {
  String upper = line;
  upper.toUpperCase();

  if (upper.startsWith("DELAY ")) {
    long ms = 0;
    if (!parseLongInRange(line.substring(6), 0, MAX_COMMAND_DELAY_MS, ms)) {
      error = "Invalid DELAY";
      return false;
    }
    return waitWithAbort(ms);
  }

  if (upper.startsWith("STRING ")) {
    typeString(line.substring(7));
    return true;
  }
  if (upper.startsWith("STRINGLN ")) {
    typeString(line.substring(9));
    pressKey(KEY_ENTER);
    return true;
  }

  if (upper.startsWith("MOUSE_MOVE ")) {
    String args = line.substring(11);
    args.trim();
    int spaceIdx = args.indexOf(' ');
    if (spaceIdx <= 0) {
      error = "MOUSE_MOVE requires dx and dy";
      return false;
    }

    long dx = 0;
    long dy = 0;
    if (!parseLongInRange(args.substring(0, spaceIdx), -127, 127, dx) ||
        !parseLongInRange(args.substring(spaceIdx + 1), -127, 127, dy)) {
      error = "MOUSE_MOVE values must be between -127 and 127";
      return false;
    }
    mouseMove((int8_t)dx, (int8_t)dy);
    return true;
  }

  if (upper == "MOUSE_CLICK" || upper.startsWith("MOUSE_CLICK ")) {
    String arg = line.length() > 11 ? line.substring(11) : "";
    arg.trim();
    arg.toUpperCase();
    if (arg.isEmpty() || arg == "LEFT") {
      mouseClick(0);
    } else if (arg == "RIGHT") {
      mouseClick(1);
    } else if (arg == "MIDDLE") {
      mouseClick(2);
    } else {
      error = "Unknown MOUSE_CLICK button";
      return false;
    }
    return true;
  }

  if (upper.startsWith("MOUSE_SCROLL ")) {
    long amount = 0;
    if (!parseLongInRange(line.substring(13), -127, 127, amount)) {
      error = "MOUSE_SCROLL value must be between -127 and 127";
      return false;
    }
    mouseScroll((int8_t)amount);
    return true;
  }

  uint8_t singleKey = resolveKey(upper);
  if (singleKey != KEY_NONE && upper.indexOf(' ') < 0) {
    pressKey(singleKey);
    return true;
  }

  uint8_t modMask = 0;
  String remaining = line;

  while (!remaining.isEmpty()) {
    remaining.trim();
    int spaceIdx = remaining.indexOf(' ');
    String token =
        spaceIdx >= 0 ? remaining.substring(0, spaceIdx) : remaining;
    remaining = spaceIdx >= 0 ? remaining.substring(spaceIdx + 1) : "";

    String upperToken = token;
    upperToken.toUpperCase();
    uint8_t mod = resolveModifier(upperToken);
    if (mod != MOD_NONE) {
      modMask |= mod;
      continue;
    }

    uint8_t key = resolveKey(upperToken);
    if (key != KEY_NONE) {
      pressKey(key, modMask);
      return true;
    }

    if (token.length() == 1) {
      KeyMapping km = getKeyMapping(token.charAt(0));
      if (km.keycode != KEY_NONE) {
        pressKey(km.keycode, modMask | km.modifier);
        return true;
      }
    }

    error = "Unknown key or command";
    return false;
  }

  if (modMask != 0) {
    pressKey(KEY_NONE, modMask);
    return true;
  }

  error = "Unknown command";
  return false;
}

// ================================================================
//  Key and Modifier Resolution
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
  if (keyName == "PAGEUP" || keyName == "PAGE_UP")
    return KEY_PAGE_UP;
  if (keyName == "PAGEDOWN" || keyName == "PAGE_DOWN")
    return KEY_PAGE_DOWN;
  if (keyName == "UP" || keyName == "UPARROW")
    return KEY_UP_ARROW;
  if (keyName == "DOWN" || keyName == "DOWNARROW")
    return KEY_DOWN_ARROW;
  if (keyName == "LEFT" || keyName == "LEFTARROW")
    return KEY_LEFT_ARROW;
  if (keyName == "RIGHT" || keyName == "RIGHTARROW")
    return KEY_RIGHT_ARROW;
  if (keyName == "CAPSLOCK" || keyName == "CAPS_LOCK")
    return KEY_CAPSLOCK;
  if (keyName == "PRINTSCREEN" || keyName == "PRINT_SCREEN")
    return KEY_PRINT_SCREEN;
  if (keyName == "SCROLLLOCK" || keyName == "SCROLL_LOCK")
    return KEY_SCROLL_LOCK;
  if (keyName == "PAUSE" || keyName == "BREAK")
    return KEY_PAUSE;
  if (keyName == "NUMLOCK" || keyName == "NUM_LOCK")
    return KEY_NUM_LOCK;
  if (keyName == "MENU" || keyName == "APP")
    return KEY_MENU;

  for (int f = 1; f <= 12; f++) {
    if (keyName == "F" + String(f)) {
      return KEY_F1 + f - 1;
    }
  }

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
