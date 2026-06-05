// ============================================================
//  USB HID — Keyboard & Mouse Emulation (ESP32-S3)
// ============================================================

#include "usb_hid.h"
#include "config.h"
#include "storage_manager.h"

#include <USB.h>
#include <USBHIDKeyboard.h>
#include <USBHIDMouse.h>

// --- Singleton HID instances ---
static USBHIDKeyboard Kbd;
static USBHIDMouse Mse;
static bool sUsbReady = false;

static constexpr uint8_t RAW_KEY_NONE = 0x00;
static constexpr uint8_t HID_MOD_LEFT_CTRL = 0x01;
static constexpr uint8_t HID_MOD_LEFT_SHIFT = 0x02;
static constexpr uint8_t HID_MOD_LEFT_ALT = 0x04;
static constexpr uint8_t HID_MOD_LEFT_GUI = 0x08;
static constexpr uint8_t HID_MOD_RIGHT_CTRL = 0x10;
static constexpr uint8_t HID_MOD_RIGHT_SHIFT = 0x20;
static constexpr uint8_t HID_MOD_RIGHT_ALT = 0x40;
static constexpr uint8_t HID_MOD_RIGHT_GUI = 0x80;

// ----------------------------------------------------------------
void initUSB() {
  if (sUsbReady) {
    return;
  }

  uint16_t vid, pid;
  getUsbIdentity(vid, pid);

  USB.VID(vid);
  USB.PID(pid);
  USB.manufacturerName(USB_MANUFACTURER);
  USB.productName(USB_PRODUCT);

  Kbd.begin();
  Mse.begin();
  USB.begin();

  // Small delay for host OS to enumerate the device
  delay(500);
  sUsbReady = true;
}

// ----------------------------------------------------------------
bool usbIsReady() { return sUsbReady; }

// ----------------------------------------------------------------
void fixLayout() {
  if (!sUsbReady) {
    return;
  }

  // ALT + SHIFT toggles keyboard layout on Windows (and many Linux DEs)
  Kbd.press(KEY_LEFT_ALT);
  Kbd.press(KEY_LEFT_SHIFT);
  delay(FIX_LAYOUT_DELAY);
  Kbd.releaseAll();
  delay(50);
}

// ----------------------------------------------------------------
void typeString(const String &text) {
  if (!sUsbReady) {
    return;
  }

  for (size_t i = 0; i < text.length(); i++) {
    char c = text.charAt(i);

    if (c == '\n') {
      Kbd.press(KEY_RETURN);
      Kbd.releaseAll();
      delay(10);
      continue;
    }
    if (c == '\t') {
      Kbd.press(KEY_TAB);
      Kbd.releaseAll();
      delay(10);
      continue;
    }

    // Use the Arduino HID library's built-in write for simplicity
    // It handles US layout ASCII natively
    Kbd.write((uint8_t)c);
    delay(5); // small inter-key delay for reliability
  }
}

// ----------------------------------------------------------------
void pressKey(uint8_t keycode, uint8_t modifier) {
  if (!sUsbReady) {
    return;
  }

  if (modifier & HID_MOD_LEFT_CTRL)
    Kbd.press(KEY_LEFT_CTRL);
  if (modifier & HID_MOD_LEFT_SHIFT)
    Kbd.press(KEY_LEFT_SHIFT);
  if (modifier & HID_MOD_LEFT_ALT)
    Kbd.press(KEY_LEFT_ALT);
  if (modifier & HID_MOD_LEFT_GUI)
    Kbd.press(KEY_LEFT_GUI);
  if (modifier & HID_MOD_RIGHT_CTRL)
    Kbd.press(KEY_RIGHT_CTRL);
  if (modifier & HID_MOD_RIGHT_SHIFT)
    Kbd.press(KEY_RIGHT_SHIFT);
  if (modifier & HID_MOD_RIGHT_ALT)
    Kbd.press(KEY_RIGHT_ALT);
  if (modifier & HID_MOD_RIGHT_GUI)
    Kbd.press(KEY_RIGHT_GUI);

  if (keycode != RAW_KEY_NONE) {
    Kbd.pressRaw(keycode);
  }

  delay(20);
  Kbd.releaseAll();
  delay(10);
}

// ----------------------------------------------------------------
void pressCombo(uint8_t keycode, uint8_t mod1, uint8_t mod2, uint8_t mod3) {
  uint8_t combined = mod1 | mod2 | mod3;
  pressKey(keycode, combined);
}

// ----------------------------------------------------------------
void releaseAllKeys() {
  if (sUsbReady) {
    Kbd.releaseAll();
  }
}

// ----------------------------------------------------------------
void mouseMove(int8_t dx, int8_t dy) {
  if (!sUsbReady) {
    return;
  }

  Mse.move(dx, dy, 0);
  delay(10);
}

// ----------------------------------------------------------------
void mouseClick(uint8_t button) {
  if (!sUsbReady) {
    return;
  }

  switch (button) {
  case 1:
    Mse.click(MOUSE_RIGHT);
    break;
  case 2:
    Mse.click(MOUSE_MIDDLE);
    break;
  default:
    Mse.click(MOUSE_LEFT);
    break;
  }
  delay(20);
}

// ----------------------------------------------------------------
void mouseScroll(int8_t amount) {
  if (!sUsbReady) {
    return;
  }

  Mse.move(0, 0, amount);
  delay(10);
}
