/**
 * @file RS485SlaveConfig.h
 * @brief RS-485 Slave Configuration for CockpitOS
 * @version 2.0 - Complete rewrite with bare-metal UART and ISR-driven RX
 *
 * To enable RS-485 slave mode, add to Config.h:
 *   #define RS485_SLAVE_ENABLED 1
 *   #define RS485_SLAVE_ADDRESS 1    // Unique address 1-126
 *
 * NOTE: RS485_MASTER_ENABLED and RS485_SLAVE_ENABLED are mutually exclusive!
 *
 * ==========================================================================
 * PROTOCOL COMPATIBILITY
 * ==========================================================================
 *
 * This slave implementation is 100% compatible with:
 *   - Arduino DCS-BIOS RS485 Master (DcsBiosNgRS485Master)
 *   - CockpitOS RS485 Master (SMART and RELAY modes)
 *   - ESP32 DCS-BIOS Library RS485 Master
 *
 * ==========================================================================
 * PROTOCOL SUMMARY
 * ==========================================================================
 *
 * Master -> Slave (Broadcast):  [Addr=0][MsgType][Length][Data...][Checksum]
 * Master -> Slave (Poll):       [Addr=N][MsgType][Length=0] <- NO CHECKSUM!
 * Slave -> Master (No data):    [0x00] <- Single byte, NO CHECKSUM!
 * Slave -> Master (With data):  [Length][MsgType][Data...][Checksum]
 *
 * CRITICAL: When Length=0, there is NO checksum byte!
 *
 * Length = number of DATA bytes (does NOT include MsgType)
 * Checksum = XOR of all bytes, or fixed 0x72 for Arduino compatibility
 *
 * ==========================================================================
 * ARCHITECTURE: Bare-Metal UART with ISR-Driven RX
 * ==========================================================================
 *
 * This implementation uses direct hardware register access:
 *   - periph_module_enable() for UART clock
 *   - uart_ll_* functions for direct register manipulation
 *   - esp_intr_alloc() for RX interrupt with FIFO threshold of 1
 *   - Echo prevention: disable RX int -> TX -> wait idle -> flush -> re-enable
 *   - RISC-V memory barriers for ESP32-C3/C6
 *   - Zero blocking calls in the hot path
 *
 * ==========================================================================
 */

#ifndef RS485_SLAVE_CONFIG_H
#define RS485_SLAVE_CONFIG_H

// ============================================================================
// SLAVE ADDRESS (REQUIRED - must be unique on the bus!)
// ============================================================================
// Set RS485_SLAVE_ADDRESS in Config.h, not here.
// Valid range: 1-126 (address 0 is reserved for broadcast)

#ifndef RS485_SLAVE_ADDRESS
#error "RS485_SLAVE_ADDRESS must be defined in Config.h (valid range: 1-126)"
#endif

#if RS485_SLAVE_ADDRESS < 1 || RS485_SLAVE_ADDRESS > 126
#error "RS485_SLAVE_ADDRESS must be between 1 and 126"
#endif

// ============================================================================
// HARDWARE PINS
// ============================================================================
//
// OPTION 1: Built-in RS485 transceiver (Waveshare ESP32-S3-RS485-CAN, etc.)
//   TX = GPIO17, RX = GPIO18, DE = GPIO21
//
// OPTION 2: External MAX485 with manual direction control
//   ESP32 TX -> MAX485 DI
//   ESP32 RX <- MAX485 RO
//   ESP32 GPIO -> MAX485 DE+RE (tied together)
//
// OPTION 3: Auto-direction RS485 module
//   Set RS485_DE_PIN to -1 (auto-direction mode)
//

// UART number (1 or 2 - UART0 is typically used for USB/debug)
#ifndef RS485_UART_NUM
#define RS485_UART_NUM          1
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

#ifndef RS485_ARDUINO_COMPAT
#define RS485_ARDUINO_COMPAT    1       // 1 = Arduino compatible (0x72 checksum)
#endif                                  // 0 = Calculated XOR checksum

// ============================================================================
// BUFFER SIZES
// ============================================================================

// TX buffer for outgoing input commands (queued until polled)
#ifndef RS485_TX_BUFFER_SIZE
#define RS485_TX_BUFFER_SIZE    128
#endif

// RX ring buffer size (ISR writes, loop reads)
#ifndef RS485_RX_BUFFER_SIZE
#define RS485_RX_BUFFER_SIZE    512
#endif

// Export data buffer (for broadcast packets)
#ifndef RS485_EXPORT_BUFFER_SIZE
#define RS485_EXPORT_BUFFER_SIZE 512
#endif

// ============================================================================
// TIMING SETTINGS
// ============================================================================

// RX timeout (microseconds) - max time to wait for packet completion
#ifndef RS485_RX_TIMEOUT_US
#define RS485_RX_TIMEOUT_US     5000
#endif

// ============================================================================
// FREERTOS TASK OPTIONS
// ============================================================================
//
// RS485_USE_TASK enables running RS485 slave in a dedicated FreeRTOS task
// instead of being called from the main loop. This provides:
//   • Deterministic response timing (critical for 2ms master timeout!)
//   • No missed polls due to main loop delays
//   • Better reliability on single-core chips (S2, C3, C6)
//
// 1 = FreeRTOS task (recommended for production)
// 0 = Main loop (simpler, good for debugging)
//

#if RS485_USE_TASK

// Task priority (higher = more important, max typically 24)
// Should be higher than main loop to ensure we respond within master's timeout
#ifndef RS485_TASK_PRIORITY
#define RS485_TASK_PRIORITY     5
#endif

// Task stack size in bytes
#ifndef RS485_TASK_STACK_SIZE
#define RS485_TASK_STACK_SIZE   4096
#endif

// Task tick interval (how often to check for packets)
// 1 = every tick (~1ms), but slave is interrupt-driven so this is just for housekeeping
#ifndef RS485_TASK_TICK_INTERVAL
#define RS485_TASK_TICK_INTERVAL 1
#endif

// Core affinity for dual-core ESP32s (S3, classic ESP32)
// 0 = Core 0, 1 = Core 1, tskNO_AFFINITY = any core
// On single-core (S2, C3, C6), this is ignored
#ifndef RS485_TASK_CORE
#define RS485_TASK_CORE         0
#endif

#endif // RS485_USE_TASK

// ============================================================================
// DEBUG OPTIONS
// ============================================================================

// Log every packet (VERY verbose - for debugging only)
#ifndef RS485_DEBUG_VERBOSE
#define RS485_DEBUG_VERBOSE     0
#endif

// Status print interval (milliseconds, 0 = disabled)
#ifndef RS485_STATUS_INTERVAL_MS
#define RS485_STATUS_INTERVAL_MS 5000
#endif

#endif // RS485_SLAVE_CONFIG_H
