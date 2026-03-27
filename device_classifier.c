/**
 * @file device_classifier.c
 * @brief Device Classification Module Implementation
 * 
 * Advanced device classification and fingerprinting
 */

#include "device_classifier.h"
#include <string.h>
#include <furi_hal.h>

#define TAG "DeviceClassifier"

/* ==================== Static Variables ==================== */

static KnownDevice g_known_devices[CLASSIFIER_MAX_KNOWN_DEVICES];
static uint16_t g_known_count = 0;

/* ==================== Simple Hash Function ==================== */

static uint32_t device_classifier_hash_string(const char* str) {
    // DJB2 hash algorithm
    uint32_t hash = 5381;
    int c;
    
    while((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    
    return hash;
}

/* ==================== Public Functions ==================== */

bool device_classifier_init(void) {
    memset(g_known_devices, 0, sizeof(g_known_devices));
    g_known_count = 0;
    
    // Load known devices from storage
    device_classifier_load_known_devices();
    
    FURI_LOG_I(TAG, "Device classifier initialized, %d known devices loaded", g_known_count);
    return true;
}

void device_classifier_deinit(void) {
    // Save known devices before deinitializing
    device_classifier_save_known_devices();
}

bool device_classifier_generate_fingerprint(const TrackedDevice* device, char* hash, size_t hash_len) {
    furi_assert(device);
    furi_assert(hash);
    furi_assert(hash_len >= CLASSIFIER_FINGERPRINT_HASH_LEN);
    
    // Generate fingerprint from MAC/UID
    uint32_t h = device_classifier_hash_string(device->mac_or_uid);
    
    // Format as hex string
    snprintf(hash, hash_len, "%08X", (unsigned int)h);
    
    return true;
}

bool device_classifier_is_known_device(const TrackedDevice* device) {
    furi_assert(device);
    
    char fingerprint[CLASSIFIER_FINGERPRINT_HASH_LEN];
    device_classifier_generate_fingerprint(device, fingerprint, sizeof(fingerprint));
    
    for(uint16_t i = 0; i < g_known_count; i++) {
        if(strcmp(g_known_devices[i].fingerprint, fingerprint) == 0) {
            return true;
        }
    }
    
    return false;
}

bool device_classifier_register_device(const TrackedDevice* device) {
    furi_assert(device);
    
    if(g_known_count >= CLASSIFIER_MAX_KNOWN_DEVICES) {
        FURI_LOG_W(TAG, "Known device table full");
        return false;
    }
    
    // Check if already registered
    if(device_classifier_is_known_device(device)) {
        return false;
    }
    
    KnownDevice* kd = &g_known_devices[g_known_count];
    
    device_classifier_generate_fingerprint(device, kd->fingerprint, sizeof(kd->fingerprint));
    strncpy(kd->device_name, device->device_name, sizeof(kd->device_name) - 1);
    kd->device_type = device->device_type;
    kd->first_seen = device->first_seen;
    kd->last_seen = device->last_seen;
    kd->encounter_count = 1;
    
    g_known_count++;
    
    FURI_LOG_I(TAG, "Registered new device: %s", device->mac_or_uid);
    
    // Save to storage
    device_classifier_save_known_devices();
    
    return true;
}

uint16_t device_classifier_get_known_count(void) {
    return g_known_count;
}

const KnownDevice* device_classifier_get_known_device(uint16_t index) {
    if(index >= g_known_count) {
        return NULL;
    }
    return &g_known_devices[index];
}

DeviceType device_classifier_refine_classification(const TrackedDevice* device) {
    furi_assert(device);
    
    DeviceType refined_type = device->device_type;
    
    // If device is known, use stored classification
    if(device_classifier_is_known_device(device)) {
        char fingerprint[CLASSIFIER_FINGERPRINT_HASH_LEN];
        device_classifier_generate_fingerprint(device, fingerprint, sizeof(fingerprint));
        
        for(uint16_t i = 0; i < g_known_count; i++) {
            if(strcmp(g_known_devices[i].fingerprint, fingerprint) == 0) {
                refined_type = g_known_devices[i].device_type;
                break;
            }
        }
    }
    
    // Additional heuristics could be applied here
    // For example, based on signal strength patterns, timing, etc.
    
    return refined_type;
}

int8_t device_classifier_get_signal_trend(const TrackedDevice* device) {
    furi_assert(device);
    
    // This would require historical RSSI data
    // For now, return 0 (stable)
    // A full implementation would track RSSI over time
    // and calculate trend using linear regression
    
    UNUSED(device);
    return 0;
}

uint8_t device_classifier_rssi_to_proximity(int8_t rssi) {
    // RSSI to proximity categories
    // These thresholds are approximate and environment-dependent
    
    if(rssi == 0) {
        return 0;  // Unknown (NFC or no RSSI)
    }
    
    if(rssi >= -40) {
        return 3;  // Very close (<1m)
    } else if(rssi >= -60) {
        return 2;  // Close (<3m)
    } else if(rssi >= -75) {
        return 1;  // Medium (3-10m)
    } else {
        return 0;  // Far (>10m)
    }
}

bool device_classifier_load_known_devices(void) {
    // In a full implementation, this would load from SD card
    // For now, we'll just clear the table
    g_known_count = 0;
    return true;
}

bool device_classifier_save_known_devices(void) {
    // In a full implementation, this would save to SD card
    // For now, we'll just return success
    return true;
}
