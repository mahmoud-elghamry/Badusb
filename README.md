# ESP32-S3 HID Lab

Firmware for an ESP32-S3 lab device that can enumerate as a USB HID keyboard
and mouse, store small DuckyScript-style payloads in LittleFS, and expose a
local Wi-Fi web panel for authorized testing.

This project is intended for devices and systems you own or are explicitly
authorized to test. The web API is protected by a local admin token and the
previous unauthenticated loot/exfiltration endpoints have been removed.

## Hardware

- Board: ESP32-S3 DevKitC-1 compatible board
- Filesystem: LittleFS in the `littlefs` partition
- BOOT button: GPIO0
- Status LED: GPIO2, adjust in `src/config.h` if your board differs

The default PlatformIO board reports as `ESP32-S3-DevKitC-1-N8` with 8 MB flash
and no PSRAM. The partition table uses a 4 MB-compatible layout.

## Build And Flash

If PlatformIO is in your PATH:

```bash
pio run
pio run -t upload
pio run -t uploadfs
```

On this workstation PlatformIO was available at:

```powershell
& 'C:\Users\malgh\.platformio\penv\Scripts\platformio.exe' run
& 'C:\Users\malgh\.platformio\penv\Scripts\platformio.exe' run -t uploadfs
```

## Boot Modes

Config mode:

1. Hold BOOT during reset or power-on for the full safety window.
2. Connect to Wi-Fi SSID `HIDLab_XXXX`.
3. Open `http://192.168.4.1`.
4. Create the local admin token on first use.

USB HID is intentionally inactive in Config mode. The panel can manage payloads
and settings, but Run and Live Execute will be rejected until the device boots
with HID active.

HID mode:

1. Reset without holding BOOT.
2. The firmware initializes USB HID, waits for enumeration, then runs the
   configured autorun payload if one exists.
3. The Wi-Fi panel remains available for authenticated management.

## Web API

All sensitive API routes require:

```text
X-Admin-Token: your-local-token
```

Available endpoints:

| Method | Endpoint | Description |
| --- | --- | --- |
| GET | `/api/status` | Device status, setup state, HID readiness |
| POST | `/api/setup` | Create admin token, only before setup |
| GET | `/api/payloads` | List payloads |
| GET | `/api/payloads/:name` | Read payload |
| POST | `/api/payloads` | Save payload |
| DELETE | `/api/payloads/:name` | Delete payload |
| POST | `/api/execute/:name` | Execute stored payload when HID is active |
| POST | `/api/execute/live` | Execute temporary script when HID is active |
| POST | `/api/stop` | Stop the running script |
| GET | `/api/settings` | Read USB ID and autorun setting |
| POST | `/api/settings` | Update USB ID or autorun setting |

Payload names are limited to simple basenames using letters, digits, `.`, `_`,
and `-`, with a maximum length of 48 characters.

## Supported Script Commands

```text
REM comment
DELAY 1000
DEFAULT_DELAY 100
STRING Hello
STRINGLN Hello
ENTER
TAB
ESCAPE
SPACE
BACKSPACE
DELETE
GUI r
CTRL ALT DELETE
SHIFT TAB
ALT F4
F1 through F12
UP / DOWN / LEFT / RIGHT
HOME / END / PAGEUP / PAGEDOWN
CAPSLOCK / NUMLOCK / SCROLLLOCK
PRINTSCREEN / PAUSE
INSERT / MENU
REPEAT 3
MOUSE_MOVE 20 -10
MOUSE_CLICK LEFT
MOUSE_CLICK RIGHT
MOUSE_CLICK MIDDLE
MOUSE_SCROLL 5
```

Invalid commands now put the parser into an error state and expose the message
through `/api/status` and the web panel.

## Project Layout

```text
platformio.ini
partitions.csv
data/www/
  index.html
  style.css
  app.js
src/
  main.cpp
  config.h
  usb_hid.h / usb_hid.cpp
  keyboard_layout.h
  ducky_parser.h / ducky_parser.cpp
  storage_manager.h / storage_manager.cpp
  wifi_manager.h / wifi_manager.cpp
  web_server.h / web_server.cpp
```
