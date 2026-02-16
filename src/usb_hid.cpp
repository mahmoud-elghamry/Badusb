// ============================================================
//  USB HID — Keyboard & Mouse Emulation (ESP32-S3)
// ============================================================

#include "usb_hid.h"
#include "config.h"
#include "keyboard_layout.h"

#include <USB.h>
#include <USBHIDKeyboard.h>
#include <USBHIDMouse.h>

// --- Singleton HID instances ---
static USBHIDKeyboard Kbd;
static USBHIDMouse Mse;

// ----------------------------------------------------------------
void initUSB() {
  USB.VID(USB_VID);
  USB.PID(USB_PID);
  USB.manufacturerName(USB_MANUFACTURER);
  USB.productName(USB_PRODUCT);

  Kbd.begin();
  Mse.begin();
  USB.begin();

  // Small delay for host OS to enumerate the device
  delay(500);
}

// ----------------------------------------------------------------
void fixLayout() {
  // ALT + SHIFT toggles keyboard layout on Windows (and many Linux DEs)
  Kbd.press(KEY_LEFT_ALT);
  Kbd.press(KEY_LEFT_SHIFT);
  delay(FIX_LAYOUT_DELAY);
  Kbd.releaseAll();
  delay(50);
}

// ----------------------------------------------------------------
void typeString(const String &text) {
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
  if (modifier & MOD_LEFT_CTRL)
    Kbd.press(KEY_LEFT_CTRL);
  if (modifier & MOD_LEFT_SHIFT)
    Kbd.press(KEY_LEFT_SHIFT);
  if (modifier & MOD_LEFT_ALT)
    Kbd.press(KEY_LEFT_ALT);
  if (modifier & MOD_LEFT_GUI)
    Kbd.press(KEY_LEFT_GUI);
  if (modifier & MOD_RIGHT_CTRL)
    Kbd.press(KEY_RIGHT_CTRL);
  if (modifier & MOD_RIGHT_SHIFT)
    Kbd.press(KEY_RIGHT_SHIFT);
  if (modifier & MOD_RIGHT_ALT)
    Kbd.press(KEY_RIGHT_ALT);
  if (modifier & MOD_RIGHT_GUI)
    Kbd.press(KEY_RIGHT_GUI);

  if (keycode != KEY_NONE) {
    Kbd.press(keycode);
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
void releaseAllKeys() { Kbd.releaseAll(); }

// ----------------------------------------------------------------
void mouseMove(int8_t dx, int8_t dy) {
  Mse.move(dx, dy, 0);
  delay(10);
}

// ----------------------------------------------------------------
void mouseClick(uint8_t button) {
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
  Mse.move(0, 0, amount);
  delay(10);
}
