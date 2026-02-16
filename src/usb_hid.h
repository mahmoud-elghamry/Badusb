#pragma once

// ============================================================
//  USB HID — Keyboard & Mouse Emulation (ESP32-S3)
// ============================================================

#include <Arduino.h>

/// Initialize USB HID (keyboard + mouse). Call once in setup().
void initUSB();

/// Send ALT+SHIFT to switch host keyboard layout to English.
void fixLayout();

/// Type a string as keyboard input (character by character).
void typeString(const String &text);

/// Press a single HID key with optional modifiers, then release.
/// @param keycode  HID key code (from keyboard_layout.h)
/// @param modifier Modifier bitmask (MOD_LEFT_CTRL, etc.)
void pressKey(uint8_t keycode, uint8_t modifier = 0);

/// Press multiple modifier keys + a key simultaneously, then release.
void pressCombo(uint8_t keycode, uint8_t mod1, uint8_t mod2 = 0,
                uint8_t mod3 = 0);

/// Release all currently held keys.
void releaseAllKeys();

/// Move the mouse cursor by (dx, dy) pixels.
void mouseMove(int8_t dx, int8_t dy);

/// Click a mouse button.  0 = left, 1 = right, 2 = middle.
void mouseClick(uint8_t button = 0);

/// Scroll the mouse wheel. Positive = up, negative = down.
void mouseScroll(int8_t amount);
