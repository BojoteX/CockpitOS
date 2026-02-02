/**
 * @file RS485Config.h
 * @brief RS-485 Master Configuration for OpenHornet ABSIS Integration
 * 
 * To enable RS-485 master mode, add to Config.h:
 *   #define RS485_MASTER_ENABLED 1
 * 
 * ==========================================================================
 * OPERATING MODES
 * ==========================================================================
 * 
 * RS485_FILTER_ADDRESSES selects the fundamental operating mode:
 * 
 *   ┌─────────────────────────────────────────────────────────────────────┐
 *   │ FILTER=1 "SMART MODE" (default)                                    │
 *   ├─────────────────────────────────────────────────────────────────────┤
 *   │ • Parses DCS-BIOS stream, extracts address/value pairs             │
 *   │ • Filters by DcsOutputTable (only addresses your panels need)      │
 *   │ • Change detection reduces bandwidth dramatically                  │
 *   │ • Reconstructs complete frames for broadcast                       │
 *   │ • Best for production with known panel configurations              │
 *   │ • Uses: 32KB RAM for change tracking + 128-entry queue             │
 *   └─────────────────────────────────────────────────────────────────────┘
 * 
 *   ┌─────────────────────────────────────────────────────────────────────┐
 *   │ FILTER=0 "RELAY MODE" (Arduino-compatible)                         │
 *   ├─────────────────────────────────────────────────────────────────────┤
 *   │ • Raw byte relay, exactly like Arduino Mega master                 │
 *   │ • NO parsing, NO filtering - bytes in, bytes out                   │
 *   │ • Small ring buffer (256 bytes), continuous drain                  │
 *   │ • Works with ANY sim, ANY aircraft, ANY address                    │
 *   │ • Use for debugging or when you don't know addresses yet           │
 *   │ • Uses: 256 bytes RAM (minimal!)                                   │
 *   └─────────────────────────────────────────────────────────────────────┘
 * 
 */

#ifndef RS485_CONFIG_H
#define RS485_CONFIG_H

// ============================================================================
// OPERATING MODE - CHOOSE ONE
// ============================================================================
// 1 = SMART MODE: Parse + filter + change detect + rebuild frames
//     Best bandwidth, requires DcsOutputTable configuration
//
// 0 = RELAY MODE: Raw byte pump like Arduino Mega master
//     Maximum compatibility, works without any configuration
//
#ifndef RS485_FILTER_ADDRESSES
#define RS485_FILTER_ADDRESSES  1
#endif

// ============================================================================
// SMART MODE OPTIONS (only apply when FILTER_ADDRESSES=1)
// ============================================================================
#if RS485_FILTER_ADDRESSES

// Change detection (delta compression)
// 1 = Only broadcast changed values (recommended - huge bandwidth savings)
// 0 = Broadcast all filtered values every frame (rarely useful)
#ifndef RS485_CHANGE_DETECT
#define RS485_CHANGE_DETECT     1
#endif

// Change queue size (address/value pairs)
// Each change = 10 bytes on wire (sync + addr + count + value)
#ifndef RS485_CHANGE_QUEUE_SIZE
#define RS485_CHANGE_QUEUE_SIZE 128
#endif

// Minimum time between broadcasts (milliseconds)
// Batches changes for efficiency
#ifndef RS485_MIN_BROADCAST_INTERVAL_MS
#define RS485_MIN_BROADCAST_INTERVAL_MS 100
#endif

#endif // RS485_FILTER_ADDRESSES

// ============================================================================
// HARDWARE PINS (Waveshare ESP32-S3-RS485-CAN defaults)
// ============================================================================

#ifndef RS485_TX_PIN
#define RS485_TX_PIN            17      // UART TX -> RS485 DI
#endif

#ifndef RS485_RX_PIN
#define RS485_RX_PIN            18      // UART RX <- RS485 RO
#endif

// Direction control pin:
//   >= 0 : GPIO number for manual DE/RE control (half-duplex mode)
//   -1   : Auto-direction hardware (no pin needed)
#ifndef RS485_EN_PIN
// #define RS485_EN_PIN            -1
#define RS485_EN_PIN            21
#endif

// ============================================================================
// PROTOCOL SETTINGS
// ============================================================================

#ifndef RS485_BAUD
#define RS485_BAUD              250000  // Must match slave firmware
#endif

#ifndef RS485_POLL_TIMEOUT_US
#define RS485_POLL_TIMEOUT_US   2000    // 2ms timeout per slave poll
#endif

// ============================================================================
// BROADCAST SETTINGS
// ============================================================================

// Uncomment to disable broadcasts entirely (for input-only testing)
// #define RS485_DISABLE_BROADCASTS 1

// Delay after broadcast before polling (microseconds)
// Gives slaves time to process received export data
#ifndef RS485_POST_BROADCAST_DELAY_US
#define RS485_POST_BROADCAST_DELAY_US 3000
#endif

// ============================================================================
// POLLING CONFIGURATION
// ============================================================================

// Maximum slave addresses to poll (valid range: 1-126)
// Set to expected max to reduce discovery scan time
#ifndef RS485_MAX_SLAVES
// #define RS485_MAX_SLAVES        126
// #define RS485_MAX_SLAVES        32
#define RS485_MAX_SLAVES        1
#endif

// ============================================================================
// BUFFER SIZES
// ============================================================================

// Buffer for slave input commands (switch/encoder strings)
#ifndef RS485_INPUT_BUFFER_SIZE
#define RS485_INPUT_BUFFER_SIZE 64
#endif

// ============================================================================
// DEBUG OPTIONS
// ============================================================================

// Log every poll/response (VERY verbose - for debugging only)
#ifndef RS485_DEBUG_VERBOSE
#define RS485_DEBUG_VERBOSE     0
#endif

// Log only errors (suppresses normal status messages)
#ifndef RS485_DEBUG_ERRORS_ONLY
#define RS485_DEBUG_ERRORS_ONLY 0
#endif

#endif // RS485_CONFIG_H
