/**
 * @file ble_scanner.c
 * @brief BLE Scanner Module Implementation
 * 
 * Handles BLE advertising packet capture and parsing
 */

#include "ble_scanner.h"
#include <string.h>
#include <furi_hal_bt.h>
#include <ble/ble.h>

#define TAG "BleScanner"

/* ==================== Static Variables ==================== */

static bool g_scanning = false;
static NfcBleTracker* g_tracker_app = NULL;
static FuriThread* g_scan_thread = NULL;

/* ==================== BLE Callbacks ==================== */

/**
 * @brief Callback for BLE advertising packets
 */
static void ble_scanner_adv_callback(
    BleGlueContext* context,
    uint8_t* mac,
    uint8_t* data,
    size_t len,
    int8_t rssi,
    void* extra_context) {
    
    UNUSED(context);
    UNUSED(extra_context);
    
    if(!g_scanning || !g_tracker_app) {
        return;
    }
    
    // Parse the advertising packet
    BleDevice device = {0};
    
    // Copy MAC address
    memcpy(device.mac, mac, 6);
    ble_scanner_format_mac(mac, device.mac_str);
    
    // Set RSSI
    device.rssi = rssi;
    
    // Set timestamp
    device.timestamp = furi_hal_rtc_get_datetime();
    
    // Parse advertising data
    if(!ble_scanner_parse_adv_data(data, len, &device)) {
        // If parsing fails, still log the MAC and RSSI
        FURI_LOG_D(TAG, "Received BLE packet from %s, RSSI: %d", device.mac_str, rssi);
    }
    
    // Create tracked device entry
    TrackedDevice tracked = {0};
    strncpy(tracked.mac_or_uid, device.mac_str, sizeof(tracked.mac_or_uid) - 1);
    strncpy(tracked.device_name, device.name[0] ? device.name : device.mac_str, 
            sizeof(tracked.device_name) - 1);
    tracked.device_type = nfc_ble_tracker_classify_ble_device(
        device.company_id, 
        device.manufacturer_data, 
        device.manufacturer_data_len);
    tracked.rssi = rssi;
    tracked.first_seen = device.timestamp;
    tracked.last_seen = device.timestamp;
    tracked.seen_count = 1;
    tracked.is_nfc = false;
    
    // Add to tracker
    nfc_ble_tracker_add_device(g_tracker_app, &tracked);
}

/* ==================== Public Functions ==================== */

bool ble_scanner_init(void) {
    // Initialize BLE hardware
    if(!furi_hal_bt_is_alive()) {
        FURI_LOG_W(TAG, "BLE hardware not available");
        return false;
    }
    
    return true;
}

void ble_scanner_deinit(void) {
    ble_scanner_stop();
}

bool ble_scanner_start(NfcBleTracker* app) {
    furi_assert(app);
    
    if(g_scanning) {
        return true;
    }
    
    g_tracker_app = app;
    
    // Start BLE scanning thread
    g_scan_thread = furi_thread_alloc();
    furi_thread_set_name(g_scan_thread, "BleScanner");
    furi_thread_set_stack_size(g_scan_thread, 2048);
    furi_thread_set_context(g_scan_thread, app);
    furi_thread_set_callback(g_scan_thread, ble_scanner_thread);
    
    furi_thread_start(g_scan_thread);
    
    g_scanning = true;
    
    FURI_LOG_I(TAG, "BLE scanning started");
    return true;
}

void ble_scanner_stop(void) {
    if(!g_scanning) {
        return;
    }
    
    g_scanning = false;
    
    // Wait for thread to finish
    if(g_scan_thread) {
        furi_thread_join(g_scan_thread);
        furi_thread_free(g_scan_thread);
        g_scan_thread = NULL;
    }
    
    g_tracker_app = NULL;
    
    FURI_LOG_I(TAG, "BLE scanning stopped");
}

bool ble_scanner_is_running(void) {
    return g_scanning;
}

bool ble_scanner_parse_adv_data(const uint8_t* data, size_t len, BleDevice* device) {
    furi_assert(data);
    furi_assert(device);
    
    size_t pos = 0;
    
    while(pos < len) {
        // Each AD structure has: [Length] [Type] [Data...]
        uint8_t structure_len = data[pos];
        
        if(structure_len == 0 || pos + structure_len >= len) {
            break;
        }
        
        uint8_t data_type = data[pos + 1];
        uint8_t* data_ptr = (uint8_t*)&data[pos + 2];
        uint8_t data_len = structure_len - 1;
        
        switch(data_type) {
            case BleDataType_Complete_Local_Name:
            case BleDataType_Shortened_Local_Name:
                // Extract device name
                if(data_len > 0 && data_len < NFC_BLE_TRACKER_MAX_DEVICE_NAME) {
                    memcpy(device->name, data_ptr, data_len);
                    device->name[data_len] = '\0';
                }
                break;
                
            case BleDataType_Manufacturer_Specific_Data:
                // Extract manufacturer data
                if(data_len >= 2) {
                    // First 2 bytes are company ID (little endian)
                    device->company_id = data_ptr[0] | (data_ptr[1] << 8);
                    
                    // Copy remaining manufacturer data
                    size_t mfgr_len = data_len - 2;
                    if(mfgr_len > 0 && mfgr_len <= BLE_SCANNER_MAX_ADV_DATA) {
                        memcpy(device->manufacturer_data, &data_ptr[2], mfgr_len);
                        device->manufacturer_data_len = mfgr_len;
                    }
                }
                break;
                
            case BleDataType_Complete_16bit_UUID:
            case BleDataType_Incomplete_16bit_UUID:
                // Extract 16-bit UUIDs
                for(size_t i = 0; i + 1 < data_len && device->uuid_count < 10; i += 2) {
                    device->uuids[device->uuid_count++] = data_ptr[i] | (data_ptr[i + 1] << 8);
                }
                break;
                
            default:
                // Ignore other data types for now
                break;
        }
        
        pos += structure_len + 1;
    }
    
    return true;
}

bool ble_scanner_extract_name(const uint8_t* data, size_t len, char* name, size_t name_len) {
    furi_assert(data);
    furi_assert(name);
    
    size_t pos = 0;
    
    while(pos < len) {
        uint8_t structure_len = data[pos];
        
        if(structure_len == 0 || pos + structure_len >= len) {
            break;
        }
        
        uint8_t data_type = data[pos + 1];
        
        if(data_type == BleDataType_Complete_Local_Name || 
           data_type == BleDataType_Shortened_Local_Name) {
            
            uint8_t name_data_len = structure_len - 1;
            if(name_data_len > 0 && name_data_len < name_len) {
                memcpy(name, &data[pos + 2], name_data_len);
                name[name_data_len] = '\0';
                return true;
            }
        }
        
        pos += structure_len + 1;
    }
    
    return false;
}

size_t ble_scanner_extract_manufacturer_data(
    const uint8_t* data, 
    size_t len, 
    uint16_t* company_id,
    uint8_t* mfgr_data,
    size_t mfgr_data_len) {
    
    furi_assert(data);
    furi_assert(company_id);
    furi_assert(mfgr_data);
    
    size_t pos = 0;
    
    while(pos < len) {
        uint8_t structure_len = data[pos];
        
        if(structure_len == 0 || pos + structure_len >= len) {
            break;
        }
        
        uint8_t data_type = data[pos + 1];
        
        if(data_type == BleDataType_Manufacturer_Specific_Data) {
            uint8_t mfgr_len = structure_len - 1;
            
            if(mfgr_len >= 2) {
                // Extract company ID (little endian)
                *company_id = data[pos + 2] | (data[pos + 3] << 8);
                
                // Copy manufacturer data
                size_t copy_len = mfgr_len - 2;
                if(copy_len > mfgr_data_len) {
                    copy_len = mfgr_data_len;
                }
                
                memcpy(mfgr_data, &data[pos + 4], copy_len);
                return copy_len;
            }
        }
        
        pos += structure_len + 1;
    }
    
    return 0;
}

void ble_scanner_format_mac(const uint8_t* mac, char* buffer) {
    furi_assert(mac);
    furi_assert(buffer);
    
    // Format as XX:XX:XX:XX:XX:XX (reverse order for BLE little-endian)
    snprintf(buffer, NFC_BLE_TRACKER_MAC_LENGTH, 
             "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
}

/* ==================== BLE Scanner Thread ==================== */

int32_t ble_scanner_thread(void* context) {
    UNUSED(context);
    
    FURI_LOG_I(TAG, "BLE scanner thread started");
    
    // Configure BLE for scanning
    // Note: This is a simplified implementation
    // Full implementation would use the Flipper's BLE HAL properly
    
    while(g_scanning) {
        // In a real implementation, this would use the FuriHal BLE API
        // to scan for advertising packets
        
        // For now, we'll use a placeholder delay
        furi_delay_ms(100);
        
        // The actual BLE scanning would happen via interrupt callbacks
        // when advertising packets are received
    }
    
    FURI_LOG_I(TAG, "BLE scanner thread exiting");
    return 0;
}
