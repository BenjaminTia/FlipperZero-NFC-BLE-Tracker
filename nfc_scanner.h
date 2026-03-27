/**
 * @file nfc_scanner.h
 * @brief NFC Scanner Module Header
 * 
 * Handles NFC tag polling, UID extraction, and type detection
 */

#pragma once

#include <furi.h>
#include <furi_hal.h>
#include "nfc_ble_tracker.h"

/* ==================== Constants ==================== */

#define NFC_SCANNER_MAX_UID_LEN 10
#define NFC_SCANNER_POLL_INTERVAL_MS 500

/* ==================== NFC Tag Types ==================== */

typedef enum {
    NfcTagType_Unknown,
    NfcTagType_EM4100,       /* 125 kHz RFID */
    NfcTagType_MifareClassic,
    NfcTagType_MifareUltralight,
    NfcTagType_MifareDesfire,
    NfcTagType_FeliCa,
    NfcTagType_NfcV,
    NfcTagType_NfcA,
    NfcTagType_NfcF,
    NfcTagType_NfcB,
} NfcTagType;

/* ==================== Parsed NFC Tag ==================== */

typedef struct {
    NfcTagType tag_type;
    uint8_t uid[NFC_SCANNER_MAX_UID_LEN];
    uint8_t uid_len;
    char uid_str[NFC_BLE_TRACKER_UID_LENGTH];
    uint32_t atqa;
    uint8_t sak;
    uint8_t manufacturer;
    bool is_authenticated;
    uint32_t timestamp;
} NfcTag;

/* ==================== Function Prototypes ==================== */

/**
 * @brief Initialize NFC scanner
 * @return true on success
 */
bool nfc_scanner_init(void);

/**
 * @brief Deinitialize NFC scanner
 */
void nfc_scanner_deinit(void);

/**
 * @brief Start NFC scanning
 * @param app Pointer to tracker application state
 * @return true on success
 */
bool nfc_scanner_start(NfcBleTracker* app);

/**
 * @brief Stop NFC scanning
 */
void nfc_scanner_stop(void);

/**
 * @brief Check if NFC scanner is running
 * @return true if scanning
 */
bool nfc_scanner_is_running(void);

/**
 * @brief Poll for NFC tag
 * @param tag Output tag structure
 * @return true if tag detected
 */
bool nfc_scanner_poll(NfcTag* tag);

/**
 * @brief Get NFC tag type name
 * @param type Tag type enum
 * @return Human-readable type name
 */
const char* nfc_scanner_get_type_name(NfcTagType type);

/**
 * @brief Classify NFC tag as device type
 * @param tag Pointer to tag structure
 * @return DeviceType classification
 */
DeviceType nfc_scanner_classify_tag(const NfcTag* tag);

/**
 * @brief NFC scanner thread entry point
 * @param context Application state
 * @return Exit code
 */
int32_t nfc_scanner_thread(void* context);

/**
 * @brief Format UID as hex string
 * @param uid UID bytes
 * @param uid_len Length of UID
 * @param buffer Output buffer
 * @param buffer_len Size of output buffer
 */
void nfc_scanner_format_uid(const uint8_t* uid, uint8_t uid_len, char* buffer, size_t buffer_len);
