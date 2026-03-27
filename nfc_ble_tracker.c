/**
 * @file nfc_ble_tracker.c
 * @brief NFC/BLE Proximity Tracker - Main Application
 */

#include "nfc_ble_tracker.h"
#include "ble_scanner.h"
#include "nfc_scanner.h"
#include <stdlib.h>
#include <string.h>
#include <furi_hal_rtc.h>

/* ==================== Constants ==================== */

#define TAG "NfcBleTracker"

/* ==================== Global State ==================== */

static NfcBleTracker* g_tracker = NULL;

/* ==================== Forward Declarations ==================== */

static void nfc_ble_tracker_input_callback(InputEvent* event, void* context);
static void nfc_ble_tracker_start_scanning(NfcBleTracker* app);
static void nfc_ble_tracker_stop_scanning(NfcBleTracker* app);
static void nfc_ble_tracker_export_logs(NfcBleTracker* app);

/* ==================== UI Rendering ==================== */

void nfc_ble_tracker_render_callback(Canvas* canvas, void* context) {
    furi_assert(canvas);
    furi_assert(context);
    
    NfcBleTracker* app = (NfcBleTracker*)context;
    
    // Clear canvas
    canvas_clear(canvas);
    
    // Set font
    canvas_set_font(canvas, FontSecondary);
    
    // Header bar
    canvas_draw_box(canvas, 0, 0, 128, 14);
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_str(canvas, 2, 11, "NFC/BLE Tracker");
    
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontBatteryPercent);
    
    // Draw status
    if(app->scanning) {
        canvas_draw_str(canvas, 100, 11, "SCAN");
    } else {
        canvas_draw_str(canvas, 100, 11, "STOP");
    }
    
    canvas_set_font(canvas, FontSecondary);
    
    // Draw statistics
    char stat_buf[42];
    snprintf(stat_buf, sizeof(stat_buf), "BLE:%d NFC:%d Total:%d",
             app->stats.total_ble_devices,
             app->stats.total_nfc_devices,
             app->stats.unique_devices);
    canvas_draw_str(canvas, 2, 24, stat_buf);
    
    // Draw device list or info
    if(app->device_count == 0) {
        canvas_draw_str(canvas, 2, 40, "No devices detected");
        canvas_draw_str(canvas, 2, 50, "Press OK to start/stop");
        
        // Draw hint icons
        canvas_draw_line(canvas, 0, 60, 127, 60);
        canvas_draw_str_aligned(canvas, 64, 55, AlignCenter, AlignBottom, "OK: Scan | B: Export");
    } else {
        // Draw device list
        uint8_t visible_items = 4;
        uint8_t start_idx = app->scroll_offset;
        
        for(uint8_t i = 0; i < visible_items && (start_idx + i) < app->device_count; i++) {
            TrackedDevice* dev = &app->devices[start_idx + i];
            
            // Highlight selected
            if((start_idx + i) == app->selected_index) {
                canvas_draw_box(canvas, 0, 28 + (i * 12), 128, 12);
                canvas_set_color(canvas, ColorWhite);
            }
            
            // Device type icon
            const char* icon = "?";
            switch(dev->device_type) {
                case DeviceType_Phone: icon = "P"; break;
                case DeviceType_Tablet: icon = "T"; break;
                case DeviceType_Laptop: icon = "L"; break;
                case DeviceType_Wearable: icon = "W"; break;
                case DeviceType_Audio: icon = "A"; break;
                case DeviceType_NFC_Tag:
                case DeviceType_Access_Card: icon = "N"; break;
                default: icon = "?"; break;
            }
            
            char dev_str[42];
            if(dev->is_nfc) {
                snprintf(dev_str, sizeof(dev_str), "[%s] %s", icon, dev->mac_or_uid);
            } else {
                snprintf(dev_str, sizeof(dev_str), "[%s] %s %ddBm", icon, 
                         dev->device_name[0] ? dev->device_name : dev->mac_or_uid,
                         dev->rssi);
            }
            
            canvas_draw_str(canvas, 2, 37 + (i * 12), dev_str);
            
            if((start_idx + i) == app->selected_index) {
                canvas_set_color(canvas, ColorBlack);
            }
        }
        
        // Scroll indicator
        if(app->device_count > visible_items) {
            canvas_draw_line(canvas, 125, 28, 125, 27 + (visible_items * 12));
            uint8_t indicator_y = 28 + (app->scroll_offset * 12 * visible_items / app->device_count);
            canvas_draw_box(canvas, 123, indicator_y, 5, 5);
        }
        
        // Footer
        canvas_draw_line(canvas, 0, 60, 127, 60);
        canvas_draw_str_aligned(canvas, 64, 55, AlignCenter, AlignBottom, "OK: Scan | B: Export");
    }
}

/* ==================== Input Handling ==================== */

static void nfc_ble_tracker_input_callback(InputEvent* event, void* context) {
    furi_assert(context);
    
    NfcBleTracker* app = (NfcBleTracker*)context;
    
    if(event->type != InputTypeShort && event->type != InputTypeRepeat) {
        return;
    }
    
    if(event->key == InputKeyOk) {
        // Toggle scanning
        if(app->scanning) {
            nfc_ble_tracker_stop_scanning(app);
        } else {
            nfc_ble_tracker_start_scanning(app);
        }
    } else if(event->key == InputKeyUp) {
        if(app->selected_index > 0) {
            app->selected_index--;
            if(app->selected_index < app->scroll_offset) {
                app->scroll_offset = app->selected_index;
            }
        }
    } else if(event->key == InputKeyDown) {
        if(app->selected_index < app->device_count - 1) {
            app->selected_index++;
            if(app->selected_index >= app->scroll_offset + 4) {
                app->scroll_offset = app->selected_index - 3;
            }
        }
    } else if(event->key == InputKeyBack) {
        // Export logs
        nfc_ble_tracker_export_logs(app);
    }
}

/* ==================== Scanning Control ==================== */

static void nfc_ble_tracker_start_scanning(NfcBleTracker* app) {
    if(app->scanning) return;

    app->scanning = true;
    app->stats.scan_start_time = furi_hal_rtc_get_datetime();

    // Initialize log file
    nfc_ble_tracker_init_log(app);

    // Start BLE scanner if enabled
    if(app->ble_enabled) {
        ble_scanner_start(app);
    }

    // Start NFC scanner if enabled
    if(app->nfc_enabled) {
        nfc_scanner_start(app);
    }

    // Vibrate to confirm start
    notification_message(app->notifications, &sequence_single_vibro);
}

static void nfc_ble_tracker_stop_scanning(NfcBleTracker* app) {
    if(!app->scanning) return;

    app->scanning = false;
    app->stats.scan_duration = furi_hal_rtc_get_datetime() - app->stats.scan_start_time;

    // Stop BLE scanner
    if(app->ble_enabled) {
        ble_scanner_stop();
    }

    // Stop NFC scanner
    if(app->nfc_enabled) {
        nfc_scanner_stop();
    }

    // Vibrate to confirm stop
    notification_message(app->notifications, &sequence_double_vibro);
}

/* ==================== Device Management ==================== */

bool nfc_ble_tracker_add_device(NfcBleTracker* app, const TrackedDevice* device) {
    furi_assert(app);
    furi_assert(device);
    
    bool is_new = true;
    bool added = false;
    
    furi_mutex_acquire(app->device_mutex, FuriWaitForever);
    
    // Check if device already exists
    for(uint16_t i = 0; i < app->device_count; i++) {
        if(strcmp(app->devices[i].mac_or_uid, device->mac_or_uid) == 0) {
            // Update existing device
            app->devices[i].last_seen = device->last_seen;
            app->devices[i].seen_count++;
            app->devices[i].rssi = device->rssi;
            is_new = false;
            break;
        }
    }
    
    if(is_new && app->device_count < NFC_BLE_TRACKER_MAX_DEVICES) {
        // Add new device
        memcpy(&app->devices[app->device_count], device, sizeof(TrackedDevice));
        app->device_count++;
        app->stats.unique_devices++;
        
        if(device->is_nfc) {
            app->stats.total_nfc_devices++;
        } else {
            app->stats.total_ble_devices++;
        }
        
        added = true;
        
        // Log the new device
        nfc_ble_tracker_log_device(app, device, true);
    }
    
    furi_mutex_release(app->device_mutex);
    
    return added;
}

/* ==================== Device Classification ==================== */

DeviceType nfc_ble_tracker_classify_ble_device(
    uint16_t company_id, 
    const uint8_t* data, 
    size_t data_len) {
    
    // Apple devices
    if(company_id == 0x004C) {
        if(data_len >= 2) {
            // Check Apple device type from manufacturer data
            uint8_t device_type = data[0];
            if(device_type == 0x02 || device_type == 0x12) {
                return DeviceType_Phone;  // iPhone
            } else if(device_type == 0x03 || device_type == 0x13) {
                return DeviceType_Tablet;  // iPad
            } else if(device_type == 0x04 || device_type == 0x14) {
                return DeviceType_Laptop;  // Mac
            } else if(device_type == 0x07 || device_type == 0x17) {
                return DeviceType_Wearable;  // Apple Watch
            } else if(device_type == 0x08 || device_type == 0x18) {
                return DeviceType_Audio;  // AirPods
            }
        }
        return DeviceType_Phone;  // Default Apple to phone
    }
    
    // Samsung
    if(company_id == 0x0075) {
        return DeviceType_Phone;
    }
    
    // Google
    if(company_id == 0x00E0) {
        return DeviceType_Phone;
    }
    
    // Microsoft
    if(company_id == 0x0006) {
        return DeviceType_Laptop;
    }
    
    // Fitbit
    if(company_id == 0x00DA) {
        return DeviceType_Wearable;
    }
    
    // Garmin
    if(company_id == 0x00FB) {
        return DeviceType_Wearable;
    }
    
    // Default classification based on common patterns
    return DeviceType_Unknown;
}

DeviceType nfc_ble_tracker_classify_nfc_device(uint8_t tag_type) {
    // Simple classification based on tag type
    // In a real implementation, this would analyze the card data
    if(tag_type < 0x10) {
        return DeviceType_Access_Card;  // Likely access card
    }
    return DeviceType_NFC_Tag;  // Generic NFC tag
}

/* ==================== Logging ==================== */

bool nfc_ble_tracker_init_log(NfcBleTracker* app) {
    furi_assert(app);
    
    // Create log directory
    storage_simply_mkdir_recursive(app->storage, NFC_BLE_TRACKER_LOG_DIR);
    
    // Generate filename with timestamp
    uint32_t now = furi_hal_rtc_get_datetime();
    snprintf(app->current_log_file, sizeof(app->current_log_file),
             "%s/track_%lu.csv", NFC_BLE_TRACKER_LOG_DIR, now);
    
    // Write CSV header if file doesn't exist
    File* file = storage_common_alloc(app->storage);
    if(storage_file_open(file, app->current_log_file, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        const char* header = "Timestamp,Type,MAC/UID,Name,DeviceType,RSSI,FirstSeen,Count\n";
        storage_file_write(file, header, strlen(header));
        storage_file_close(file);
        storage_common_free(app->storage, file);
        return true;
    }
    
    storage_common_free(app->storage, file);
    return false;
}

bool nfc_ble_tracker_log_device(NfcBleTracker* app, const TrackedDevice* device, bool is_new) {
    furi_assert(app);
    furi_assert(device);
    
    File* file = storage_common_alloc(app->storage);
    if(!storage_file_open(file, app->current_log_file, FSAM_WRITE, FSOM_OPEN_APPEND)) {
        storage_common_free(app->storage, file);
        return false;
    }
    
    char line[256];
    const char* type_str = device->is_nfc ? "NFC" : "BLE";
    const char* device_type_str = "";
    
    switch(device->device_type) {
        case DeviceType_Phone: device_type_str = "Phone"; break;
        case DeviceType_Tablet: device_type_str = "Tablet"; break;
        case DeviceType_Laptop: device_type_str = "Laptop"; break;
        case DeviceType_Wearable: device_type_str = "Wearable"; break;
        case DeviceType_Audio: device_type_str = "Audio"; break;
        case DeviceType_IoT: device_type_str = "IoT"; break;
        case DeviceType_NFC_Tag: device_type_str = "NFC_Tag"; break;
        case DeviceType_Access_Card: device_type_str = "Access_Card"; break;
        default: device_type_str = "Unknown"; break;
    }
    
    snprintf(line, sizeof(line), "%lu,%s,%s,%s,%s,%d,%lu,%d\n",
             device->last_seen,
             type_str,
             device->mac_or_uid,
             device->device_name,
             device_type_str,
             device->rssi,
             device->first_seen,
             device->seen_count);
    
    storage_file_write(file, line, strlen(line));
    storage_file_close(file);
    storage_common_free(app->storage, file);
    
    return true;
}

void nfc_ble_tracker_export_logs(NfcBleTracker* app) {
    furi_assert(app);
    
    // Show export notification
    notification_message(app->notifications, &sequence_single_vibro);
    
    // In full implementation, this would copy logs to a more accessible location
    // or display a summary on screen
}

/* ==================== Initialization ==================== */

bool nfc_ble_tracker_init(NfcBleTracker* app) {
    furi_assert(app);
    
    memset(app, 0, sizeof(NfcBleTracker));
    
    // Initialize GUI
    app->gui = furi_record_open(RECORD_GUI);
    if(!app->gui) {
        FURI_LOG_E(TAG, "Failed to open GUI");
        return false;
    }
    
    // Initialize Storage
    app->storage = furi_record_open(RECORD_STORAGE);
    if(!app->storage) {
        FURI_LOG_E(TAG, "Failed to open Storage");
        furi_record_close(RECORD_GUI);
        return false;
    }
    
    // Initialize Notifications
    app->notifications = furi_record_open(RECORD_NOTIFICATION);
    if(!app->notifications) {
        FURI_LOG_E(TAG, "Failed to open Notification");
        furi_record_close(RECORD_STORAGE);
        furi_record_close(RECORD_GUI);
        return false;
    }
    
    // Create mutex
    app->device_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    if(!app->device_mutex) {
        FURI_LOG_E(TAG, "Failed to create mutex");
        furi_record_close(RECORD_NOTIFICATION);
        furi_record_close(RECORD_STORAGE);
        furi_record_close(RECORD_GUI);
        return false;
    }
    
    // Enable features by default
    app->ble_enabled = true;
    app->nfc_enabled = true;
    
    // Set global pointer
    g_tracker = app;
    
    return true;
}

void nfc_ble_tracker_free(NfcBleTracker* app) {
    furi_assert(app);
    
    // Stop scanning
    nfc_ble_tracker_stop_scanning(app);
    
    // Free mutex
    if(app->device_mutex) {
        furi_mutex_free(app->device_mutex);
    }
    
    // Close records
    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_STORAGE);
    furi_record_close(RECORD_GUI);
}

/* ==================== Helper Functions ==================== */

void nfc_ble_tracker_format_time(uint32_t timestamp, char* buffer, size_t buffer_size) {
    // Convert Unix timestamp to readable format
    // Note: This is a simplified implementation
    snprintf(buffer, buffer_size, "%lu", timestamp);
}

float nfc_ble_tracker_rssi_to_distance(int8_t rssi) {
    // Simplified RSSI to distance conversion
    // Actual calibration would require empirical testing
    const int8_t RSSI_AT_1M = -59;  // Typical RSSI at 1 meter
    const float N = 2.0f;  // Path loss exponent
    
    if(rssi == 0) {
        return 0.0f;  // Unknown distance
    }
    
    float ratio = (float)(RSSI_AT_1M - rssi) / (10.0f * N);
    return powf(10.0f, ratio);
}

/* ==================== Main Application Entry ==================== */

int32_t nfc_ble_tracker_app(void* p) {
    UNUSED(p);
    
    // Allocate application state
    NfcBleTracker* app = malloc(sizeof(NfcBleTracker));
    if(!app) {
        FURI_LOG_E(TAG, "Failed to allocate memory");
        return 1;
    }
    
    // Initialize
    if(!nfc_ble_tracker_init(app)) {
        FURI_LOG_E(TAG, "Failed to initialize");
        free(app);
        return 1;
    }
    
    // Create GUI view
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, nfc_ble_tracker_render_callback, app);

    // Register with GUI
    Gui* gui = app->gui;
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    // Input event queue
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    view_port_input_callback_set(view_port, nfc_ble_tracker_input_callback, app);
    
    // Initialize scanners
    ble_scanner_init();
    nfc_scanner_init();

    // Main event loop
    InputEvent event;
    while(true) {
        // Wait for input event with timeout
        if(furi_message_queue_get(event_queue, &event, 100) == FuriStatusOk) {
            nfc_ble_tracker_input_callback(&event, app);
        }
        
        // Update UI
        view_port_update(view_port);
        
        // Check for exit condition (long press Back)
        // Could be implemented here
    }

    // Cleanup
    ble_scanner_deinit();
    nfc_scanner_deinit();
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);
    nfc_ble_tracker_free(app);
    free(app);

    return 0;
}
