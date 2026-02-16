# ⚡ BadUSB ESP32-S3

A feature-rich BadUSB device built on the **ESP32-S3** with native USB HID emulation, **DuckyScript** interpreter, on-device payload storage, and a **Wi-Fi web interface** for remote payload management.

## Features

| Feature | Description |
|---------|-------------|
| 🎹 HID Keyboard | Full USB keyboard emulation with US layout |
| 🖱️ HID Mouse | Mouse movement, clicks, and scroll |
| 📜 DuckyScript | Compatible interpreter with extended commands |
| 📡 Wi-Fi AP | Built-in access point with captive portal |
| 🌐 Web Panel | Dark-themed dashboard for payload management |
| 💾 LittleFS | On-device script storage (~2 MB) |
| ⚡ Live Execute | Run DuckyScript commands in real-time |
| 🔄 Auto-Run | Configure payloads to execute on boot |
| 🛡️ Safety Mode | Hold BOOT button to prevent payload execution |
| 🔤 Layout Fix | Auto-switches host keyboard to English (ALT+SHIFT) |

## Hardware

- **Board**: ESP32-S3 DevKitC-1 (or compatible)
- **Flash**: 4 MB
- **PSRAM**: 2 MB

## Quick Start

### 1. Build & Flash Firmware

```bash
# Build firmware
pio run

# Flash firmware
pio run -t upload

# Upload web UI to LittleFS
pio run -t uploadfs
```

### 2. Config Mode (Upload Payloads)

1. Hold the **BOOT** button during reset/power-on
2. The LED stays solid — Config Mode active
3. Connect to Wi-Fi: **BadUSB_XXXX** (password: `badusb1234`)
4. Open **http://192.168.4.1** in your browser
5. Create, edit, and upload DuckyScript payloads
6. Set an auto-run payload in Settings

### 3. Attack Mode (Execute Payloads)

1. Reset **without** holding BOOT
2. The device auto-runs the configured payload
3. Wi-Fi is still active in the background for remote control

## DuckyScript Reference

```
REM This is a comment
DELAY 1000
STRING Hello World
STRINGLN Hello World (with Enter)
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
F1 - F12
UP / DOWN / LEFT / RIGHT
HOME / END / PAGEUP / PAGEDOWN
CAPSLOCK / NUMLOCK / SCROLLLOCK
PRINTSCREEN / PAUSE
INSERT / MENU
DEFAULT_DELAY 100
REPEAT 3
MOUSE_MOVE 100 50
MOUSE_CLICK LEFT
MOUSE_CLICK RIGHT
MOUSE_CLICK MIDDLE
MOUSE_SCROLL 5
```

## Project Structure

```
hack USB/
├── platformio.ini          # PlatformIO configuration
├── partitions.csv          # Custom partition table (4MB)
├── data/www/               # Web UI (LittleFS)
│   ├── index.html
│   ├── style.css
│   └── app.js
└── src/
    ├── main.cpp            # Entry point + boot safety
    ├── config.h            # Global configuration
    ├── usb_hid.h / .cpp    # USB HID keyboard & mouse
    ├── keyboard_layout.h   # US HID scan codes
    ├── ducky_parser.h/.cpp # DuckyScript interpreter (FreeRTOS)
    ├── storage_manager.h/.cpp # LittleFS CRUD
    ├── wifi_manager.h/.cpp # Wi-Fi AP + captive portal
    └── web_server.h / .cpp # REST API + static serving
```

## API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/payloads` | List all payloads |
| GET | `/api/payloads/:name` | Get payload content |
| POST | `/api/payloads` | Save payload |
| DELETE | `/api/payloads/:name` | Delete payload |
| POST | `/api/execute/:name` | Execute stored payload |
| POST | `/api/execute/live` | Execute script from body |
| POST | `/api/stop` | Abort running script |
| GET | `/api/status` | Device status & info |
| POST | `/api/settings` | Update settings |

## Configuration

Edit `src/config.h` to customize:
- USB VID/PID and device name
- Wi-Fi SSID prefix and password
- LED and button pin assignments
- Parser task priority and stack size

## License

For educational and authorized security testing purposes only.
