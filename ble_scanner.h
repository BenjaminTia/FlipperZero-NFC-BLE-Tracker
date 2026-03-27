/**
 * @file ble_scanner.h
 * @brief BLE Scanner Module Header
 * 
 * Handles BLE advertising packet capture and parsing
 */

#pragma once

#include <furi.h>
#include <furi_hal.h>
#include "nfc_ble_tracker.h"

/* ==================== Constants ==================== */

#define BLE_SCANNER_MAX_ADV_DATA 31
#define BLE_SCANNER_TIMEOUT_MS 1000

/* ==================== BLE Advertisement Data Types ==================== */

typedef enum {
    BleDataType_Flags = 0x01,
    BleDataType_Incomplete_16bit_UUID = 0x02,
    BleDataType_Complete_16bit_UUID = 0x03,
    BleDataType_Incomplete_32bit_UUID = 0x04,
    BleDataType_Complete_32bit_UUID = 0x05,
    BleDataType_Incomplete_128bit_UUID = 0x06,
    BleDataType_Complete_128bit_UUID = 0x07,
    BleDataType_Shortened_Local_Name = 0x08,
    BleDataType_Complete_Local_Name = 0x09,
    BleDataType_Tx_Power_Level = 0x0A,
    BleDataType_Class_of_Device = 0x0D,
    BleDataType_Simple_Pairing_Hash = 0x0E,
    BleDataType_Simple_Pairing_Randomizer = 0x0F,
    BleDataType_Device_ID = 0x10,
    BleDataType_Security_Manager_TK = 0x11,
    BleDataType_Security_Manager_OOB_Flags = 0x12,
    BleDataType_Peripheral_Connection_Interval = 0x13,
    BleDataType_Service_Data_16bit = 0x16,
    BleDataType_Manufacturer_Specific_Data = 0xFF,
} BleDataType;

/* ==================== Parsed BLE Device ==================== */

typedef struct {
    uint8_t mac[6];
    char mac_str[NFC_BLE_TRACKER_MAC_LENGTH];
    char name[NFC_BLE_TRACKER_MAX_DEVICE_NAME];
    int8_t rssi;
    uint16_t company_id;
    uint8_t manufacturer_data[BLE_SCANNER_MAX_ADV_DATA];
    size_t manufacturer_data_len;
    uint16_t uuids[10];
    size_t uuid_count;
    uint32_t timestamp;
} BleDevice;

/* ==================== Function Prototypes ==================== */

/**
 * @brief Initialize BLE scanner
 * @return true on success
 */
bool ble_scanner_init(void);

/**
 * @brief Deinitialize BLE scanner
 */
void ble_scanner_deinit(void);

/**
 * @brief Start BLE scanning
 * @param app Pointer to tracker application state
 * @return true on success
 */
bool ble_scanner_start(NfcBleTracker* app);

/**
 * @brief Stop BLE scanning
 */
void ble_scanner_stop(void);

/**
 * @brief Check if BLE scanner is running
 * @return true if scanning
 */
bool ble_scanner_is_running(void);

/**
 * @brief Parse BLE advertising data
 * @param data Raw advertising data
 * @param len Length of advertising data
 * @param device Output parsed device structure
 * @return true if parsing successful
 */
bool ble_scanner_parse_adv_data(const uint8_t* data, size_t len, BleDevice* device);

/**
 * @brief Extract device name from advertising data
 * @param data Raw advertising data
 * @param len Length of advertising data
 * @param name Output buffer for device name
 * @param name_len Size of output buffer
 * @return true if name found
 */
bool ble_scanner_extract_name(const uint8_t* data, size_t len, char* name, size_t name_len);

/**
 * @brief Extract manufacturer data from advertising data
 * @param data Raw advertising data
 * @param len Length of advertising data
 * @param company_id Output company ID
 * @param mfgr_data Output manufacturer data buffer
 * @param mfgr_data_len Size of manufacturer data buffer
 * @return Length of manufacturer data extracted
 */
size_t ble_scanner_extract_manufacturer_data(
    const uint8_t* data, 
    size_t len, 
    uint16_t* company_id,
    uint8_t* mfgr_data,
    size_t mfgr_data_len);

/**
 * @brief BLE scanner thread entry point
 * @param context Application state
 * @return Exit code
 */
int32_t ble_scanner_thread(void* context);

/**
 * @brief Format MAC address as string
 * @param mac MAC address bytes (6 bytes)
 * @param buffer Output buffer (must be at least 18 bytes)
 */
void ble_scanner_format_mac(const uint8_t* mac, char* buffer);
