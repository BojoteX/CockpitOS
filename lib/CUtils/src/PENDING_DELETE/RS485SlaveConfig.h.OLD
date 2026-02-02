/**
 * @file RS485SlaveConfig.h
 * @brief RS-485 Slave Configuration for CockpitOS
 *
 * This file is ONLY included when RS485_SLAVE_ENABLED is defined in Config.h.
 *
 * To enable RS-485 slave mode, add to Config.h:
 *   #define RS485_SLAVE_ENABLED 1
 *   #define RS485_SLAVE_ADDRESS 1    // Unique address 1-126
 *
 * NOTE: RS485_MASTER_ENABLED and RS485_SLAVE_ENABLED are mutually exclusive!
 *
 * COMPATIBILITY:
 *   This slave implementation is compatible with:
 *   - Arduino DCS-BIOS RS485 Master (DcsBiosNgRS485Master)
 *   - CockpitOS RS485 Master (SMART and DUMB modes)
 *   - ESP32 RS485 Master reference implementation
 */

#ifndef RS485_SLAVE_CONFIG_H
#define RS485_SLAVE_CONFIG_H

// ============================================================================
// SLAVE ADDRESS (REQUIRED - must be unique on the bus!)
// ============================================================================

#ifndef RS485_SLAVE_ADDRESS
#define RS485_SLAVE_ADDRESS     1       // Valid range: 1-126
#endif

// Sanity check
#if RS485_SLAVE_ADDRESS < 1 || RS485_SLAVE_ADDRESS > 126
#error "RS485_SLAVE_ADDRESS must be between 1 and 126"
#endif

// ============================================================================
// HARDWARE PINS
// ============================================================================
// For ESP32 with external MAX485 module, typical wiring:
//   ESP32 TX (GPIO17) -> MAX485 DI
//   ESP32 RX (GPIO16) -> MAX485 RO
//   ESP32 GPIO (GPIO4) -> MAX485 DE+RE (tied together)
//
// For Waveshare ESP32-S3-RS485-CAN board, use the built-in RS485:
//   TX = GPIO17, RX = GPIO18, EN = GPIO21

#ifndef RS485_TX_PIN
#define RS485_TX_PIN            17      // UART TX -> MAX485 DI
#endif

#ifndef RS485_RX_PIN
#define RS485_RX_PIN            18      // UART RX <- MAX485 RO
#endif

#ifndef RS485_EN_PIN
#define RS485_EN_PIN            21      // Direction control -> MAX485 DE+RE
#endif

// ============================================================================
// PROTOCOL SETTINGS (must match master - do not change!)
// ============================================================================

#ifndef RS485_BAUD
#define RS485_BAUD              250000  // 250kbps - DCS-BIOS standard
#endif

// ============================================================================
// COMPATIBILITY MODE
// ============================================================================
// Arduino DCS-BIOS slave uses fixed 0x72 as checksum placeholder.
// Set to 1 for maximum compatibility with Arduino masters.
// Set to 0 to use proper calculated XOR checksum.
//
// NOTE: CockpitOS master accepts both, so either setting works with it.

#ifndef RS485_SLAVE_ARDUINO_COMPAT
#define RS485_SLAVE_ARDUINO_COMPAT  1   // 1 = Arduino compatible (0x72 checksum)
#endif                                  // 0 = Calculated XOR checksum

// ============================================================================
// BUFFER SIZES
// ============================================================================

#ifndef RS485_TX_BUFFER_SIZE
#define RS485_TX_BUFFER_SIZE    128     // Buffer for outgoing input commands
#endif

// ============================================================================
// TIMING SETTINGS
// ============================================================================

// These are tuned for the DCS-BIOS RS485 protocol and generally
// should not need to be changed.

// Sync gap detection (microseconds) - gap > this indicates new packet
#ifndef RS485_SLAVE_SYNC_GAP_US
#define RS485_SLAVE_SYNC_GAP_US     500
#endif

// RX timeout (microseconds) - max time to wait for packet completion
#ifndef RS485_SLAVE_RX_TIMEOUT_US
#define RS485_SLAVE_RX_TIMEOUT_US   5000
#endif

// ============================================================================
// DEBUG OPTIONS
// ============================================================================

#ifndef RS485_SLAVE_DEBUG_VERBOSE
#define RS485_SLAVE_DEBUG_VERBOSE   0   // 1 = Log every poll (very verbose!)
#endif

#endif // RS485_SLAVE_CONFIG_H
