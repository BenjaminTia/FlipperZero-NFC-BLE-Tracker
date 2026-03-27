/**
 * @file nfc_ble_tracker.h
 * @brief NFC/BLE Proximity Tracker - Main Header
 * 
 * A passive tracking device that logs all NFC and BLE devices
 * that come near the Flipper Zero with timestamps and signal strength.
 */

#pragma once

#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <storage/storage.h>
#include <notification/notification.h>
#include <input/input.h>

/* ==================== Configuration ==================== */

#define NFC_BLE_TRACKER_VERSION "1.0.0"
#define NFC_BLE_TRACKER_LOG_DIR "/ext/nfc_ble_logs"
#define NFC_BLE_TRACKER_MAX_DEVICES 100
#define NFC_BLE_TRACKER_MAX_DEVICE_NAME 32
#define NFC_BLE_TRACKER_MAC_LENGTH 17
#define NFC_BLE_TRACKER_UID_LENGTH 16

/* ==================== Data Structures ==================== */

/**
 * @brief Device type classification
 */
typedef enum {
    DeviceType_Unknown,
    DeviceType_Phone,
    DeviceType_Tablet,
    DeviceType_Laptop,
    DeviceType_Wearable,
    DeviceType_Audio,
    DeviceType_IoT,
    DeviceType_NFC_Tag,
    DeviceType_Access_Card,
} DeviceType;

/**
 * @brief Tracked device entry
 */
typedef struct {
    char mac_or_uid[NFC_BLE_TRACKER_MAC_LENGTH];
    char device_name[NFC_BLE_TRACKER_MAX_DEVICE_NAME];
    DeviceType device_type;
    int8_t rssi;           /* Signal strength (BLE only, -127 to 0) */
    uint32_t first_seen;   /* Timestamp in seconds since epoch */
    uint32_t last_seen;    /* Timestamp in seconds since epoch */
    uint16_t seen_count;   /* Number of times detected */
    bool is_nfc;           /* true = NFC, false = BLE */
} TrackedDevice;

/**
 * @brief Tracker statistics
 */
typedef struct {
    uint16_t total_ble_devices;
    uint16_t total_nfc_devices;
    uint16_t unique_devices;
    uint32_t scan_start_time;
    uint32_t scan_duration;
} TrackerStats;

/**
 * @brief Main application state
 */
typedef struct {
    Gui* gui;
    Storage* storage;
    NotificationApp* notifications;
    
    FuriMutex* device_mutex;
    TrackedDevice devices[NFC_BLE_TRACKER_MAX_DEVICES];
    uint16_t device_count;
    
    TrackerStats stats;
    
    bool scanning;
    bool ble_enabled;
    bool nfc_enabled;
    
    uint8_t selected_index;
    uint8_t scroll_offset;
    
    char current_log_file[64];
} NfcBleTracker;

/* ==================== Function Prototypes ==================== */

/**
 * @brief Initialize the tracker application
 * @param app Pointer to application state
 * @return true on success
 */
bool nfc_ble_tracker_init(NfcBleTracker* app);

/**
 * @brief Free tracker resources
 * @param app Pointer to application state
 */
void nfc_ble_tracker_free(NfcBleTracker* app);

/**
 * @brief Add or update a tracked device
 * @param app Pointer to application state
 * @param device Pointer to device data
 * @return true if new device added
 */
bool nfc_ble_tracker_add_device(NfcBleTracker* app, const TrackedDevice* device);

/**
 * @brief Get device type from BLE manufacturer data
 * @param company_id BLE company ID
 * @param data Manufacturer specific data
 * @param data_len Length of manufacturer data
 * @return DeviceType classification
 */
DeviceType nfc_ble_tracker_classify_ble_device(
    uint16_t company_id, 
    const uint8_t* data, 
    size_t data_len);

/**
 * @brief Get device type from NFC tag
 * @param tag_type NFC tag type
 * @return DeviceType classification
 */
DeviceType nfc_ble_tracker_classify_nfc_device(uint8_t tag_type);

/**
 * @brief Initialize log file for current session
 * @param app Pointer to application state
 * @return true on success
 */
bool nfc_ble_tracker_init_log(NfcBleTracker* app);

/**
 * @brief Write device entry to log file
 * @param app Pointer to application state
 * @param device Pointer to device data
 * @param is_new true if new device, false if update
 * @return true on success
 */
bool nfc_ble_tracker_log_device(NfcBleTracker* app, const TrackedDevice* device, bool is_new);

/**
 * @brief Main UI rendering callback
 * @param canvas Canvas to draw on
 * @param context Application state
 */
void nfc_ble_tracker_render_callback(Canvas* canvas, void* context);

/**
 * @brief BLE scanner thread entry point
 * @param context Application state
 */
int32_t nfc_ble_tracker_ble_thread(void* context);

/**
 * @brief NFC scanner thread entry point
 * @param context Application state
 */
int32_t nfc_ble_tracker_nfc_thread(void* context);

/**
 * @brief Format timestamp as human-readable string
 * @param timestamp Unix timestamp
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 */
void nfc_ble_tracker_format_time(uint32_t timestamp, char* buffer, size_t buffer_size);

/**
 * @brief Convert RSSI to proximity estimate
 * @param rssi RSSI value in dBm
 * @return Approximate distance in meters
 */
float nfc_ble_tracker_rssi_to_distance(int8_t rssi);
