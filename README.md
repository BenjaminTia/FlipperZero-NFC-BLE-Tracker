# NFC/BLE Tracker for Flipper Zero

![Status](https://img.shields.io/badge/status-in%20development-orange)
![License](https://img.shields.io/badge/license-MIT-blue)
![Platform](https://img.shields.io/badge/platform-Flipper%20Zero-orange)
![Language](https://img.shields.io/badge/language-C-blue)

A Flipper Zero application that scans for nearby NFC cards and BLE devices, logs detected devices, and classifies them by type. Currently in active development.

## Current Status

> **Work in Progress** — Core NFC scanning is functional but has known issues. BLE scanning requires additional hardware. Proximity estimation is not yet implemented.

### Known Issues
- **NFC Duplicate Logging** — NFC cards are detected but scanned repeatedly instead of deduplicating (see [Issue #1](../../issues/1))
- **BLE Requires WiFi Dev Board** — BLE scanning does not work on stock Flipper Zero hardware; a GPIO-connected WiFi dev board is required (see [Issue #2](../../issues/2))
- **Proximity Estimation Not Implemented** — RSSI-based distance calculation is planned but not yet built (see [Issue #3](../../issues/3))

## Features

### Working
- **NFC Card Detection** — Polls for NFC-A tags (ISO 14443A) and reads UID
- **Device Classification** — Identifies device types based on BLE manufacturer data
- **Device Logging** — Logs detected devices with timestamps to SD card
- **Custom UI** — Scrollable device list with BLE/NFC counts

### In Progress / Planned
- BLE scanning (requires WiFi dev board via GPIO)
- Proximity estimation via RSSI
- NFC deduplication fix
- CSV export to `/ext/nfc_ble_logs/`

## Project Structure

```
FlipperZero-NFC-BLE-Tracker/
├── application.fam         # Flipper app manifest
├── nfc_ble_tracker.h       # Main header with data structures
├── nfc_ble_tracker.c       # Main application and UI
├── ble_scanner.h           # BLE scanner module header
├── ble_scanner.c           # BLE advertising packet capture
├── nfc_scanner.h           # NFC scanner module header
├── nfc_scanner.c           # NFC tag polling and detection
├── device_classifier.h     # Device classification header
├── device_classifier.c     # Device type identification
├── icon.png                # App icon (10x10)
├── nfc_ble_tracker.png     # App preview image
├── .gitignore              # Ignores build artifacts
├── CHANGELOG.md            # Version history
└── README.md               # This file
```

## Building

### Prerequisites
- Flipper Zero UFBT (Universal Flipper Build Tool)
- `arm-none-eabi-gcc` toolchain

### Build Instructions

```bash
# Install ufbt
pip install ufbt

# Clone the repo
git clone https://github.com/BenjaminTia/FlipperZero-NFC-BLE-Tracker.git
cd FlipperZero-NFC-BLE-Tracker

# Build and deploy to connected Flipper Zero
ufbt launch
```

## Usage

1. **Launch the app** from the Flipper Zero app menu
2. **Press OK** to start scanning
3. **View detected devices** in real-time on the display
4. **Navigate** with Up/Down arrows to scroll through the device list
5. **Press Back** to stop scanning and return to menu

### Display
- Header shows app name and scan status
- Statistics row shows NFC and BLE counts
- Device list shows type prefix (`[N]` = NFC, `[B]` = BLE) and device identifier

## Technical Details

### NFC Scanning
- Polls for NFC-A tags (ISO 14443A)
- Reads UID on detection
- Classifies tag type (Mifare Classic, Ultralight, DESFire, generic)

### BLE Scanning
- Parses BLE advertising packets for device name and manufacturer data
- Uses company ID lookup for device classification
- **Requires WiFi dev board connected via GPIO** — does not function on base Flipper Zero hardware

### Device Classification

| Company ID | Manufacturer | Classification |
|------------|--------------|----------------|
| 0x004C | Apple | Phone/Tablet/Watch |
| 0x0075 | Samsung | Phone |
| 0x00E0 | Google | Phone |
| 0x0006 | Microsoft | Laptop |

## Portfolio / Resume Value

This project demonstrates:
- **Embedded C programming** on resource-constrained hardware
- **Wireless protocol knowledge** — NFC (ISO 14443A) and BLE advertising
- **Flipper Zero SDK** usage and app architecture
- **Hardware integration** — GPIO peripheral interfacing
- **Debugging & iteration** — identifying and documenting real hardware limitations

## Limitations

- NFC deduplication not yet implemented (cards are logged on every poll cycle)
- BLE requires Flipper WiFi dev board (GPIO connection)
- Proximity estimation via RSSI is not implemented
- NFC polling limited to NFC-A type tags

## Future Enhancements

- [ ] Fix NFC duplicate detection
- [ ] Implement BLE scanning via WiFi dev board GPIO
- [ ] Add RSSI-based proximity estimation
- [ ] CSV log export to SD card
- [ ] Support NFC-B and NFC-F tag types

## License

MIT License

## Disclaimer

This tool is intended for educational and personal research purposes only. Always obtain proper authorization before scanning wireless devices.
