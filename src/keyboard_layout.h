#pragma once

// ============================================================
//  Keyboard Layout — US HID Scan Codes
// ============================================================
//  Reference: USB HID Usage Tables (Keyboard/Keypad Page 0x07)
// ============================================================

#include <cstdint>

// --- Modifier bit masks (bitmap for modifier byte) ---
#define MOD_NONE 0x00
#define MOD_LEFT_CTRL 0x01
#define MOD_LEFT_SHIFT 0x02
#define MOD_LEFT_ALT 0x04
#define MOD_LEFT_GUI 0x08
#define MOD_RIGHT_CTRL 0x10
#define MOD_RIGHT_SHIFT 0x20
#define MOD_RIGHT_ALT 0x40
#define MOD_RIGHT_GUI 0x80

// --- HID Key Codes ---
#define KEY_NONE 0x00
#define KEY_A 0x04
#define KEY_B 0x05
#define KEY_C 0x06
#define KEY_D 0x07
#define KEY_E 0x08
#define KEY_F 0x09
#define KEY_G 0x0A
#define KEY_H 0x0B
#define KEY_I 0x0C
#define KEY_J 0x0D
#define KEY_K 0x0E
#define KEY_L 0x0F
#define KEY_M 0x10
#define KEY_N 0x11
#define KEY_O 0x12
#define KEY_P 0x13
#define KEY_Q 0x14
#define KEY_R 0x15
#define KEY_S 0x16
#define KEY_T 0x17
#define KEY_U 0x18
#define KEY_V 0x19
#define KEY_W 0x1A
#define KEY_X 0x1B
#define KEY_Y 0x1C
#define KEY_Z 0x1D
#define KEY_1 0x1E
#define KEY_2 0x1F
#define KEY_3 0x20
#define KEY_4 0x21
#define KEY_5 0x22
#define KEY_6 0x23
#define KEY_7 0x24
#define KEY_8 0x25
#define KEY_9 0x26
#define KEY_0 0x27

#define KEY_ENTER 0x28
#define KEY_ESCAPE 0x29
#define KEY_BACKSPACE 0x2A
#define KEY_TAB 0x2B
#define KEY_SPACE 0x2C
#define KEY_MINUS 0x2D
#define KEY_EQUAL 0x2E
#define KEY_LEFT_BRACE 0x2F
#define KEY_RIGHT_BRACE 0x30
#define KEY_BACKSLASH 0x31
#define KEY_SEMICOLON 0x33
#define KEY_APOSTROPHE 0x34
#define KEY_GRAVE 0x35
#define KEY_COMMA 0x36
#define KEY_PERIOD 0x37
#define KEY_SLASH 0x38

#define KEY_CAPSLOCK 0x39
#define KEY_F1 0x3A
#define KEY_F2 0x3B
#define KEY_F3 0x3C
#define KEY_F4 0x3D
#define KEY_F5 0x3E
#define KEY_F6 0x3F
#define KEY_F7 0x40
#define KEY_F8 0x41
#define KEY_F9 0x42
#define KEY_F10 0x43
#define KEY_F11 0x44
#define KEY_F12 0x45

#define KEY_PRINT_SCREEN 0x46
#define KEY_SCROLL_LOCK 0x47
#define KEY_PAUSE 0x48
#define KEY_INSERT 0x49
#define KEY_HOME 0x4A
#define KEY_PAGE_UP 0x4B
#define KEY_DELETE 0x4C
#define KEY_END 0x4D
#define KEY_PAGE_DOWN 0x4E
#define KEY_RIGHT_ARROW 0x4F
#define KEY_LEFT_ARROW 0x50
#define KEY_DOWN_ARROW 0x51
#define KEY_UP_ARROW 0x52

#define KEY_NUM_LOCK 0x53
#define KEY_MENU 0x65

// --- US ASCII → HID Key mapping ---
// Each entry: { keycode, modifier }
struct KeyMapping {
  uint8_t keycode;
  uint8_t modifier;
};

// Maps ASCII 0x20–0x7E to HID key + modifier
static const KeyMapping US_LAYOUT[] = {
    // 0x20 ' '
    {KEY_SPACE, MOD_NONE},
    // 0x21 '!'
    {KEY_1, MOD_LEFT_SHIFT},
    // 0x22 '"'
    {KEY_APOSTROPHE, MOD_LEFT_SHIFT},
    // 0x23 '#'
    {KEY_3, MOD_LEFT_SHIFT},
    // 0x24 '$'
    {KEY_4, MOD_LEFT_SHIFT},
    // 0x25 '%'
    {KEY_5, MOD_LEFT_SHIFT},
    // 0x26 '&'
    {KEY_7, MOD_LEFT_SHIFT},
    // 0x27 '\''
    {KEY_APOSTROPHE, MOD_NONE},
    // 0x28 '('
    {KEY_9, MOD_LEFT_SHIFT},
    // 0x29 ')'
    {KEY_0, MOD_LEFT_SHIFT},
    // 0x2A '*'
    {KEY_8, MOD_LEFT_SHIFT},
    // 0x2B '+'
    {KEY_EQUAL, MOD_LEFT_SHIFT},
    // 0x2C ','
    {KEY_COMMA, MOD_NONE},
    // 0x2D '-'
    {KEY_MINUS, MOD_NONE},
    // 0x2E '.'
    {KEY_PERIOD, MOD_NONE},
    // 0x2F '/'
    {KEY_SLASH, MOD_NONE},
    // 0x30–0x39 '0'–'9'
    {KEY_0, MOD_NONE},
    {KEY_1, MOD_NONE},
    {KEY_2, MOD_NONE},
    {KEY_3, MOD_NONE},
    {KEY_4, MOD_NONE},
    {KEY_5, MOD_NONE},
    {KEY_6, MOD_NONE},
    {KEY_7, MOD_NONE},
    {KEY_8, MOD_NONE},
    {KEY_9, MOD_NONE},
    // 0x3A ':'
    {KEY_SEMICOLON, MOD_LEFT_SHIFT},
    // 0x3B ';'
    {KEY_SEMICOLON, MOD_NONE},
    // 0x3C '<'
    {KEY_COMMA, MOD_LEFT_SHIFT},
    // 0x3D '='
    {KEY_EQUAL, MOD_NONE},
    // 0x3E '>'
    {KEY_PERIOD, MOD_LEFT_SHIFT},
    // 0x3F '?'
    {KEY_SLASH, MOD_LEFT_SHIFT},
    // 0x40 '@'
    {KEY_2, MOD_LEFT_SHIFT},
    // 0x41–0x5A 'A'–'Z'
    {KEY_A, MOD_LEFT_SHIFT},
    {KEY_B, MOD_LEFT_SHIFT},
    {KEY_C, MOD_LEFT_SHIFT},
    {KEY_D, MOD_LEFT_SHIFT},
    {KEY_E, MOD_LEFT_SHIFT},
    {KEY_F, MOD_LEFT_SHIFT},
    {KEY_G, MOD_LEFT_SHIFT},
    {KEY_H, MOD_LEFT_SHIFT},
    {KEY_I, MOD_LEFT_SHIFT},
    {KEY_J, MOD_LEFT_SHIFT},
    {KEY_K, MOD_LEFT_SHIFT},
    {KEY_L, MOD_LEFT_SHIFT},
    {KEY_M, MOD_LEFT_SHIFT},
    {KEY_N, MOD_LEFT_SHIFT},
    {KEY_O, MOD_LEFT_SHIFT},
    {KEY_P, MOD_LEFT_SHIFT},
    {KEY_Q, MOD_LEFT_SHIFT},
    {KEY_R, MOD_LEFT_SHIFT},
    {KEY_S, MOD_LEFT_SHIFT},
    {KEY_T, MOD_LEFT_SHIFT},
    {KEY_U, MOD_LEFT_SHIFT},
    {KEY_V, MOD_LEFT_SHIFT},
    {KEY_W, MOD_LEFT_SHIFT},
    {KEY_X, MOD_LEFT_SHIFT},
    {KEY_Y, MOD_LEFT_SHIFT},
    {KEY_Z, MOD_LEFT_SHIFT},
    // 0x5B '['
    {KEY_LEFT_BRACE, MOD_NONE},
    // 0x5C '\\'
    {KEY_BACKSLASH, MOD_NONE},
    // 0x5D ']'
    {KEY_RIGHT_BRACE, MOD_NONE},
    // 0x5E '^'
    {KEY_6, MOD_LEFT_SHIFT},
    // 0x5F '_'
    {KEY_MINUS, MOD_LEFT_SHIFT},
    // 0x60 '`'
    {KEY_GRAVE, MOD_NONE},
    // 0x61–0x7A 'a'–'z'
    {KEY_A, MOD_NONE},
    {KEY_B, MOD_NONE},
    {KEY_C, MOD_NONE},
    {KEY_D, MOD_NONE},
    {KEY_E, MOD_NONE},
    {KEY_F, MOD_NONE},
    {KEY_G, MOD_NONE},
    {KEY_H, MOD_NONE},
    {KEY_I, MOD_NONE},
    {KEY_J, MOD_NONE},
    {KEY_K, MOD_NONE},
    {KEY_L, MOD_NONE},
    {KEY_M, MOD_NONE},
    {KEY_N, MOD_NONE},
    {KEY_O, MOD_NONE},
    {KEY_P, MOD_NONE},
    {KEY_Q, MOD_NONE},
    {KEY_R, MOD_NONE},
    {KEY_S, MOD_NONE},
    {KEY_T, MOD_NONE},
    {KEY_U, MOD_NONE},
    {KEY_V, MOD_NONE},
    {KEY_W, MOD_NONE},
    {KEY_X, MOD_NONE},
    {KEY_Y, MOD_NONE},
    {KEY_Z, MOD_NONE},
    // 0x7B '{'
    {KEY_LEFT_BRACE, MOD_LEFT_SHIFT},
    // 0x7C '|'
    {KEY_BACKSLASH, MOD_LEFT_SHIFT},
    // 0x7D '}'
    {KEY_RIGHT_BRACE, MOD_LEFT_SHIFT},
    // 0x7E '~'
    {KEY_GRAVE, MOD_LEFT_SHIFT},
};

// Helper: get HID key mapping for a printable ASCII character
inline KeyMapping getKeyMapping(char c) {
  if (c >= 0x20 && c <= 0x7E) {
    return US_LAYOUT[c - 0x20];
  }
  return {KEY_NONE, MOD_NONE};
}
