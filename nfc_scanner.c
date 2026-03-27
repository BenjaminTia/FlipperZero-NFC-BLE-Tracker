/**
 * @file nfc_scanner.c
 * @brief NFC Scanner Module Implementation
 * 
 * Handles NFC tag polling, UID extraction, and type detection
 */

#include "nfc_scanner.h"
#include <string.h>
#include <furi_hal_nfc.h>

#define TAG "NfcScanner"

/* ==================== Static Variables ==================== */

static bool g_scanning = false;
static NfcBleTracker* g_tracker_app = NULL;
static FuriThread* g_scan_thread = NULL;

/* ==================== NFC Tag Type Names ==================== */

static const char* nfc_type_names[] = {
    "Unknown",
    "EM4100",
    "Mifare Classic",
    "Mifare Ultralight",
    "Mifare DESFire",
    "FeliCa",
    "NFC-V",
    "NFC-A",
    "NFC-F",
    "NFC-B",
};

/* ==================== Public Functions ==================== */

bool nfc_scanner_init(void) {
    // NFC hardware is initialized by furi_hal
    return true;
}

void nfc_scanner_deinit(void) {
    nfc_scanner_stop();
}

bool nfc_scanner_start(NfcBleTracker* app) {
    furi_assert(app);
    
    if(g_scanning) {
        return true;
    }
    
    g_tracker_app = app;
    
    // Start NFC scanning thread
    g_scan_thread = furi_thread_alloc();
    furi_thread_set_name(g_scan_thread, "NfcScanner");
    furi_thread_set_stack_size(g_scan_thread, 2048);
    furi_thread_set_context(g_scan_thread, app);
    furi_thread_set_callback(g_scan_thread, nfc_scanner_thread);
    
    furi_thread_start(g_scan_thread);
    
    g_scanning = true;
    
    FURI_LOG_I(TAG, "NFC scanning started");
    return true;
}

void nfc_scanner_stop(void) {
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
    
    FURI_LOG_I(TAG, "NFC scanning stopped");
}

bool nfc_scanner_is_running(void) {
    return g_scanning;
}

const char* nfc_scanner_get_type_name(NfcTagType type) {
    if(type >= 0 && type < sizeof(nfc_type_names) / sizeof(nfc_type_names[0])) {
        return nfc_type_names[type];
    }
    return nfc_type_names[0];
}

DeviceType nfc_scanner_classify_tag(const NfcTag* tag) {
    furi_assert(tag);
    
    // Classify based on tag type and UID patterns
    switch(tag->tag_type) {
        case NfcTagType_EM4100:
            // EM4100 is commonly used for access cards
            return DeviceType_Access_Card;
            
        case NfcTagType_MifareClassic:
            // Mifare Classic can be access cards or transit cards
            // Check UID prefix for common manufacturers
            if(tag->uid_len >= 3) {
                // NXP Mifare cards
                if(tag->uid[0] == 0x04 || tag->uid[0] == 0x08) {
                    return DeviceType_Access_Card;
                }
            }
            return DeviceType_Access_Card;
            
        case NfcTagType_MifareUltralight:
            // Ultralight often used in transit tickets
            return DeviceType_Access_Card;
            
        case NfcTagType_MifareDesfire:
            // DESFire is more secure, often used for payment/transit
            return DeviceType_Access_Card;
            
        case NfcTagType_FeliCa:
            // FeliCa used in transit systems (Suica, etc.)
            return DeviceType_Access_Card;
            
        case NfcTagType_NfcV:
        case NfcTagType_NfcA:
        case NfcTagType_NfcF:
        case NfcTagType_NfcB:
            // Generic NFC tags
            return DeviceType_NFC_Tag;
            
        default:
            return DeviceType_Unknown;
    }
}

void nfc_scanner_format_uid(const uint8_t* uid, uint8_t uid_len, char* buffer, size_t buffer_len) {
    furi_assert(uid);
    furi_assert(buffer);
    
    // Format UID as hex string without separators
    size_t pos = 0;
    for(uint8_t i = 0; i < uid_len && pos < buffer_len - 3; i++) {
        pos += snprintf(buffer + pos, buffer_len - pos, "%02X", uid[i]);
    }
    buffer[pos] = '\0';
}

bool nfc_scanner_poll(NfcTag* tag) {
    furi_assert(tag);
    
    memset(tag, 0, sizeof(NfcTag));
    
    // Try to detect NFC-A tags (most common - Mifare, etc.)
    FuriHalNfcDevData nfc_data = {0};
    FuriHalNfcProtocol protocol = 0;
    
    // Poll for NFC-A tags
    if(furi_hal_nfc_detect(&nfc_data, &protocol, 500)) {
        tag->timestamp = furi_hal_rtc_get_datetime();
        tag->atqa = nfc_data.atqa;
        tag->sak = nfc_data.sak;
        
        // Copy UID
        if(nfc_data.uid_len <= NFC_SCANNER_MAX_UID_LEN) {
            memcpy(tag->uid, nfc_data.uid, nfc_data.uid_len);
            tag->uid_len = nfc_data.uid_len;
            nfc_scanner_format_uid(tag->uid, tag->uid_len, tag->uid_str, sizeof(tag->uid_str));
        }
        
        // Determine tag type based on SAK and ATQA
        // Mifare Classic detection
        if(tag->sak == 0x08 || tag->sak == 0x18 || tag->sak == 0x88) {
            tag->tag_type = NfcTagType_MifareClassic;
        }
        // Mifare Ultralight detection
        else if(tag->sak == 0x00 && (tag->atqa == 0x0044 || tag->atqa == 0x0042)) {
            tag->tag_type = NfcTagType_MifareUltralight;
        }
        // Mifare DESFire detection
        else if(tag->sak == 0x20 && (tag->atqa == 0x0344 || tag->atqa == 0x0244)) {
            tag->tag_type = NfcTagType_MifareDesfire;
        }
        // Generic NFC-A
        else {
            tag->tag_type = NfcTagType_NfcA;
        }
        
        // Get manufacturer from UID
        if(tag->uid_len > 0) {
            tag->manufacturer = tag->uid[0];
        }
        
        FURI_LOG_D(TAG, "Detected NFC-A tag: %s, type: %s", 
                   tag->uid_str, nfc_scanner_get_type_name(tag->tag_type));
        
        return true;
    }
    
    // Note: Full implementation would also poll for NFC-B, NFC-F, NFC-V
    // For brevity, we focus on NFC-A which covers most common tags
    
    return false;
}

/* ==================== NFC Scanner Thread ==================== */

int32_t nfc_scanner_thread(void* context) {
    UNUSED(context);
    
    FURI_LOG_I(TAG, "NFC scanner thread started");
    
    while(g_scanning) {
        NfcTag tag = {0};
        
        // Poll for NFC tags
        if(nfc_scanner_poll(&tag)) {
            if(g_tracker_app) {
                // Create tracked device entry
                TrackedDevice tracked = {0};
                strncpy(tracked.mac_or_uid, tag.uid_str, sizeof(tracked.mac_or_uid) - 1);
                strncpy(tracked.device_name, tag.uid_str, sizeof(tracked.device_name) - 1);
                tracked.device_type = nfc_scanner_classify_tag(&tag);
                tracked.rssi = 0;  // NFC doesn't use RSSI
                tracked.first_seen = tag.timestamp;
                tracked.last_seen = tag.timestamp;
                tracked.seen_count = 1;
                tracked.is_nfc = true;
                
                // Add to tracker
                nfc_ble_tracker_add_device(g_tracker_app, &tracked);
                
                // Vibrate on new tag detection
                notification_message(g_tracker_app->notifications, &sequence_single_vibro);
            }
        }
        
        // Poll interval
        furi_delay_ms(NFC_SCANNER_POLL_INTERVAL_MS);
    }
    
    FURI_LOG_I(TAG, "NFC scanner thread exiting");
    return 0;
}
