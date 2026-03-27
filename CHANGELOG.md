# Changelog

All notable changes to this project will be documented here.

Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

---

## [Unreleased]

### In Progress
- Fix NFC deduplication — same tag being logged on every scan cycle (#1)
- BLE scanning via WiFi dev board GPIO connection (#2)
- Proximity estimation from RSSI using path loss model (#3)

### Planned
- `calculate_proximity()` function with distance categories (Immediate / Near / Far / Unknown)
- Rewrite `ble_scanner.c` to communicate with dev board over UART/SPI
- Proximity category added to CSV log output and on-device UI

---

## [0.1.0] - 2026-03-28

### Added
- NFC-A (ISO 14443A) tag scanning and detection
- Device classification by manufacturer ID (Apple, Samsung, Google, Microsoft, Fitbit, Garmin)
- CSV logging to SD card at `/ext/nfc_ble_logs/`
- Scrollable device list UI on the Flipper Zero 128x64 display
- Device type icons: `[P]` Phone, `[T]` Tablet, `[L]` Laptop, `[W]` Wearable, `[A]` Audio, `[N]` NFC, `[?]` Unknown
- BLE scanner module (`ble_scanner.c`) — compiles but non-functional without WiFi dev board
- RSSI recording per detected device
- Repeat detection counter (`Count` field in CSV)
- `application.fam` manifest for Flipper Zero app packaging

### Known Issues
- NFC tags are logged repeatedly on every poll cycle instead of deduplicating (#1)
- BLE scanning produces no output — requires WiFi dev board GPIO connection (#2)
- Proximity estimation documented in README but not yet implemented (#3)
