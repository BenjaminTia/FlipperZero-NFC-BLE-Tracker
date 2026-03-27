/**
 * @file device_classifier.h
 * @brief Device Classification Module Header
 * 
 * Advanced device classification and fingerprinting
 */

#pragma once

#include "nfc_ble_tracker.h"

/* ==================== Constants ==================== */

#define CLASSIFIER_MAX_KNOWN_DEVICES 50
#define CLASSIFIER_FINGERPRINT_HASH_LEN 16

/* ==================== Known Device Entry ==================== */

typedef struct {
    char fingerprint[CLASSIFIER_FINGERPRINT_HASH_LEN];
    char device_name[NFC_BLE_TRACKER_MAX_DEVICE_NAME];
    DeviceType device_type;
    uint32_t first_seen;
    uint32_t last_seen;
    uint16_t encounter_count;
} KnownDevice;

/* ==================== Function Prototypes ==================== */

/**
 * @brief Initialize device classifier
 * @return true on success
 */
bool device_classifier_init(void);

/**
 * @brief Deinitialize device classifier
 */
void device_classifier_deinit(void);

/**
 * @brief Generate fingerprint hash for a device
 * @param device Pointer to tracked device
 * @param hash Output hash buffer
 * @param hash_len Length of hash buffer
 * @return true on success
 */
bool device_classifier_generate_fingerprint(const TrackedDevice* device, char* hash, size_t hash_len);

/**
 * @brief Check if device is known (seen before)
 * @param device Pointer to tracked device
 * @return true if device is known
 */
bool device_classifier_is_known_device(const TrackedDevice* device);

/**
 * @brief Register device as known
 * @param device Pointer to tracked device
 * @return true on success
 */
bool device_classifier_register_device(const TrackedDevice* device);

/**
 * @brief Get count of known devices
 * @return Number of known devices
 */
uint16_t device_classifier_get_known_count(void);

/**
 * @brief Get known device by index
 * @param index Device index
 * @return Pointer to known device or NULL
 */
const KnownDevice* device_classifier_get_known_device(uint16_t index);

/**
 * @brief Classify device based on advanced heuristics
 * @param device Pointer to tracked device
 * @return Refined device type
 */
DeviceType device_classifier_refine_classification(const TrackedDevice* device);

/**
 * @brief Get signal strength trend for device
 * @param device Pointer to tracked device
 * @return 1 = approaching, -1 = moving away, 0 = stable
 */
int8_t device_classifier_get_signal_trend(const TrackedDevice* device);

/**
 * @brief Estimate proximity category from RSSI
 * @param rssi RSSI value
 * @return 0 = far (>10m), 1 = medium (3-10m), 2 = close (<3m), 3 = very close (<1m)
 */
uint8_t device_classifier_rssi_to_proximity(int8_t rssi);

/**
 * @brief Load known devices from storage
 * @return true on success
 */
bool device_classifier_load_known_devices(void);

/**
 * @brief Save known devices to storage
 * @return true on success
 */
bool device_classifier_save_known_devices(void);
