# NFC/BLE Proximity Tracker for Flipper Zero

A sophisticated device tracking application for the Flipper Zero that passively logs NFC and BLE devices in proximity, complete with device classification, signal strength tracking, and data export capabilities.

## Features

### Core Functionality
- **Dual-Protocol Scanning**: Simultaneously scans for BLE advertising packets and NFC tags
- **Real-Time Device Logging**: Tracks all wireless devices with timestamps
- **Device Classification**: Automatically identifies device types (phone, tablet, wearable, access card, etc.)
- **Signal Strength Tracking**: Records RSSI values for proximity estimation
- **Repeat Detection**: Identifies previously seen devices
- **CSV Export**: Logs data to SD card for further analysis

### Advanced Features
- **Device Fingerprinting**: Generates unique identifiers for device tracking
- **Proximity Estimation**: Converts RSSI to approximate distance categories
- **Known Device Database**: Maintains a persistent list of encountered devices
- **Custom UI**: Clean interface with scrollable device list and statistics

## Project Structure

```
flipper-nfc-ble-tracker/
├── application.fam          # Flipper app manifest
├── nfc_ble_tracker.h        # Main header with data structures
├── nfc_ble_tracker.c        # Main application and UI
├── ble_scanner.h            # BLE scanner module header
├── ble_scanner.c            # BLE advertising packet capture
├── nfc_scanner.h            # NFC scanner module header
├── nfc_scanner.c            # NFC tag polling and detection
├── device_classifier.h      # Device classification header
├── device_classifier.c      # Advanced device fingerprinting
└── README.md                # This file
```

## Building

### Prerequisites
- Flipper Zero firmware source code
- arm-none-eabi-gcc toolchain
- Flipper Zero UFBT (Universal Flipper Build Tool)

### Build Instructions

```bash
# Using ufbt (recommended)
ufbt launch

# Or build manually
cd flipper-nfc-ble-tracker
arm-none-eabi-gcc [flags] -o nfc_ble_tracker.fap
```

### Deploy to Flipper Zero

```bash
# Using ufbt
ufbt launch

# Or copy manually
cp nfc_ble_tracker.fap /SD/apps/Tools/
```

## Usage

### Basic Operation

1. **Launch the app** from the Flipper Zero menu
2. **Press OK** to start scanning
3. **View detected devices** in real-time on the screen
4. **Navigate** with Up/Down arrows to scroll through devices
5. **Press Back** to export logs
6. **Press OK again** to stop scanning

### Display Information

- **Header**: App name and scanning status
- **Statistics**: BLE count, NFC count, total unique devices
- **Device List**: 
  - `[P]` = Phone
  - `[T]` = Tablet
  - `[L]` = Laptop
  - `[W]` = Wearable
  - `[A]` = Audio device
  - `[N]` = NFC tag/card
  - `[?]` = Unknown

### Log File Format

Logs are saved to `/ext/nfc_ble_logs/` in CSV format:

```csv
Timestamp,Type,MAC/UID,Name,DeviceType,RSSI,FirstSeen,Count
1234567890,BLE,AA:BB:CC:DD:EE:FF,iPhone 14,Phone,-45,1234567880,5
1234567891,NFC,04A2B3C4D5,,Access_Card,0,1234567891,1
```

## Technical Details

### BLE Scanning
- Captures BLE advertising packets (37/38/39 channels)
- Parses manufacturer data for device classification
- Extracts device names from advertising data
- Records RSSI for proximity estimation

### NFC Scanning
- Polls for NFC-A tags (ISO 14443A)
- Supports Mifare Classic, Ultralight, DESFire
- Reads UID and tag type
- Classifies as access card or generic tag

### Device Classification

| Company ID | Manufacturer | Classification |
|------------|--------------|----------------|
| 0x004C | Apple | Phone/Tablet/Watch/Audio |
| 0x0075 | Samsung | Phone |
| 0x00E0 | Google | Phone |
| 0x0006 | Microsoft | Laptop |
| 0x00DA | Fitbit | Wearable |
| 0x00FB | Garmin | Wearable |

### RSSI to Distance

Approximate distance estimation using path loss model:

```
distance = 10^((RSSI_at_1m - RSSI) / (10 * N))
```

Where:
- RSSI_at_1m = -59 dBm (calibrated)
- N = 2.0 (path loss exponent)

## Portfolio/Resume Value

This project demonstrates:

1. **Embedded Systems Programming**: Real-time multitasking on resource-constrained hardware
2. **Wireless Protocol Expertise**: Deep understanding of BLE and NFC protocols
3. **Data Structures**: Efficient device tracking with hash-based lookups
4. **File I/O**: SD card logging with CSV formatting
5. **UI Development**: Custom embedded UI with scrolling lists
6. **Security Research**: Tool for wireless security auditing

### Keywords for Resume
- Flipper Zero Development
- BLE Advertising Protocol
- NFC/RFID Technology
- Embedded C Programming
- Real-Time Systems
- Wireless Security Auditing
- Device Fingerprinting

## Limitations

- BLE scanning uses Flipper's built-in BLE radio (requires WiFi devboard for full functionality)
- NFC polling is limited to NFC-A tags (most common type)
- RSSI-based distance is environment-dependent and approximate
- Device classification is based on manufacturer data and may not be 100% accurate

## Future Enhancements

- [ ] Add NFC-B, NFC-F, NFC-V support
- [ ] Implement BLE GATT scanning for more device info
- [ ] Add signal strength graph visualization
- [ ] Implement mesh networking for multi-Flipper tracking
- [ ] Add web interface for log visualization
- [ ] Support for external GPS for location tagging
- [ ] Machine learning for improved device classification

## License

MIT License - See LICENSE file for details

## Credits

Developed for educational and security research purposes.

## Disclaimer

This tool is intended for legitimate security research, authorized penetration testing, and educational purposes only. Always obtain proper authorization before scanning or monitoring wireless devices.
