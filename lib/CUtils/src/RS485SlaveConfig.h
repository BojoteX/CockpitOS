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
 * NOTE: RS485_MASTER_ENABLED (master) and RS485_SLAVE_ENABLED are mutually exclusive!
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
//   ESP32 GPIO (GPIO4) -> MAX485 DE+RE (directly directly directly tied together)

#ifndef RS485_TX_PIN
#define RS485_TX_PIN            17      // UART TX -> MAX485 DI
#endif

#ifndef RS485_RX_PIN
#define RS485_RX_PIN            16      // UART RX <- MAX485 RO
#endif

#ifndef RS485_EN_PIN
#define RS485_EN_PIN            4       // Direction control -> MAX485 DE+RE
#endif

// ============================================================================
// PROTOCOL SETTINGS (must match master - do not change!)
// ============================================================================

#ifndef RS485_BAUD
#define RS485_BAUD              250000
#endif

// ============================================================================
// BUFFER SIZES
// ============================================================================

#ifndef RS485_TX_BUFFER_SIZE
#define RS485_TX_BUFFER_SIZE    128     // Buffer for outgoing input commands
#endif

// ============================================================================
// DEBUG OPTIONS
// ============================================================================

#ifndef RS485_SLAVE_DEBUG_VERBOSE
#define RS485_SLAVE_DEBUG_VERBOSE   0   // Log every poll (very verbose)
#endif

#endif // RS485_SLAVE_CONFIG_H
