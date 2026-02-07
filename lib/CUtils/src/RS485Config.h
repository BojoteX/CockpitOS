/**
 * @file RS485Config.h
 * @brief RS-485 Master Configuration for CockpitOS
 * @version 2.0 - Complete rewrite with bare-metal UART and ISR-driven RX
 *
 * To enable RS-485 master mode, add to Config.h:
 *   #define RS485_MASTER_ENABLED 1
 *
 * ==========================================================================
 * OPERATING MODES
 * ==========================================================================
 *
 * RS485_SMART_MODE selects the fundamental operating mode:
 *
 *   ┌─────────────────────────────────────────────────────────────────────┐
 *   │ SMART_MODE=1 (default) - Intelligent Filtered Broadcasting         │
 *   ├─────────────────────────────────────────────────────────────────────┤
 *   │ • Parses DCS-BIOS stream, extracts address/value pairs             │
 *   │ • Filters by DcsOutputTable (only addresses your slaves need)      │
 *   │ • Change detection reduces bandwidth by 100-1000x                  │
 *   │ • Reconstructs valid DCS-BIOS frames for broadcast                 │
 *   │ • Best for production with known panel configurations              │
 *   │ • Uses: 32KB RAM for change tracking + change queue                │
 *   └─────────────────────────────────────────────────────────────────────┘
 *
 *   ┌─────────────────────────────────────────────────────────────────────┐
 *   │ SMART_MODE=0 - Raw Relay Mode (Arduino-compatible)                 │
 *   ├─────────────────────────────────────────────────────────────────────┤
 *   │ • Raw byte relay, exactly like Arduino Mega master                 │
 *   │ • NO parsing, NO filtering - bytes in, bytes out                   │
 *   │ • Works with ANY sim, ANY aircraft, ANY address                    │
 *   │ • Use for debugging or when you don't know addresses yet           │
 *   │ • Uses: 512 bytes RAM (minimal!)                                   │
 *   └─────────────────────────────────────────────────────────────────────┘
 *
 * ==========================================================================
 * ARCHITECTURE: Bare-Metal UART with ISR-Driven RX
 * ==========================================================================
 *
 * This implementation uses direct hardware register access for maximum
 * performance and minimum latency:
 *
 *   • periph_module_enable() for UART clock
 *   • uart_ll_* functions for direct register manipulation
 *   • esp_intr_alloc() for RX interrupt with FIFO threshold of 1
 *   • No ESP-IDF uart_driver overhead
 *   • No blocking calls in the hot path
 *   • RISC-V memory barriers for ESP32-C3/C6 cache coherency
 *
 * ==========================================================================
 */

#ifndef RS485_CONFIG_H
#define RS485_CONFIG_H

// Enables running RS485 in a dedicated FreeRTOS task
#ifndef RS485_USE_TASK
#define RS485_USE_TASK			1 
#endif

// Larger = more efficient, smaller = more responsive polling
#ifndef RS485_MAX_BROADCAST_CHUNK                  64 
#define RS485_MAX_BROADCAST_CHUNK                  64 
#endif

// ============================================================================
// SMART MODE OPTIONS (only apply when SMART_MODE=1)
// ============================================================================
#if RS485_SMART_MODE

// Change detection (delta compression)
// 1 = Only broadcast changed values (recommended - huge bandwidth savings)
// 0 = Broadcast all filtered values every frame (rarely useful)
#ifndef RS485_CHANGE_DETECT
#define RS485_CHANGE_DETECT     1
#endif

// Change queue size (address/value pairs)
// Each entry = 4 bytes RAM, each change on wire = ~10 bytes
#ifndef RS485_CHANGE_QUEUE_SIZE
#define RS485_CHANGE_QUEUE_SIZE 128
#endif

// Maximum bytes per broadcast chunk
// Larger = more efficient, smaller = more responsive polling
#ifndef RS485_MAX_BROADCAST_CHUNK
#define RS485_MAX_BROADCAST_CHUNK 64
#endif

#endif // RS485_SMART_MODE

// ============================================================================
// RELAY MODE OPTIONS (only apply when SMART_MODE=0)
// ============================================================================
#if !RS485_SMART_MODE

// Ring buffer size for raw export data
// 512 bytes = ~2 full DCS-BIOS frames
#ifndef RS485_RAW_BUFFER_SIZE
#define RS485_RAW_BUFFER_SIZE   512
#endif

// Maximum bytes per broadcast in relay mode
#ifndef RS485_RELAY_CHUNK_SIZE
#define RS485_RELAY_CHUNK_SIZE  128
#endif

#endif // !RS485_SMART_MODE

// ============================================================================
// HARDWARE PINS
// ============================================================================
//
// OPTION 1: Built-in RS485 transceiver (Waveshare ESP32-S3-RS485-CAN, etc.)
//   TX = GPIO17, RX = GPIO18, DE = GPIO21
//   Set RS485_DE_PIN to the board's direction control pin
//
// OPTION 2: External MAX485 with manual direction control
//   ESP32 TX -> MAX485 DI
//   ESP32 RX <- MAX485 RO
//   ESP32 GPIO -> MAX485 DE+RE (tied together)
//   Set RS485_DE_PIN to the GPIO controlling DE/RE
//
// OPTION 3: Auto-direction RS485 module
//   These boards automatically switch TX/RX direction.
//   Only TX and RX pins needed - no direction control!
//   Set RS485_DE_PIN to -1 (auto-direction mode)
//

// UART number (1 or 2 - UART0 is typically used for USB/debug)
#ifndef RS485_UART_NUM
#define RS485_UART_NUM          1
#endif

// ============================================================================
// PROTOCOL SETTINGS
// ============================================================================

#ifndef RS485_BAUD
#define RS485_BAUD              250000  // Must match all devices on bus
#endif

// Poll timeout (microseconds) - how long to wait for FIRST response byte
// Matches standalone POLL_TIMEOUT_US = 1000 (1ms)
#ifndef RS485_POLL_TIMEOUT_US
#define RS485_POLL_TIMEOUT_US   1000
#endif

// RX timeout (microseconds) - how long to wait for complete message
// Matches standalone RX_TIMEOUT_US = 5000 (5ms)
#ifndef RS485_RX_TIMEOUT_US
#define RS485_RX_TIMEOUT_US     5000
#endif

// Maximum microseconds between polls (controls broadcast chunk timing)
#ifndef RS485_MAX_POLL_INTERVAL_US
#define RS485_MAX_POLL_INTERVAL_US 2000
#endif

// MessageBuffer drain timeout (microseconds)
// If a completed message hasn't been drained within this time, force-clear it.
// Prevents stalled message processing from blocking bus polls.
// Borrowed from AVR MasterPCConnection::checkTimeout() (5ms safety valve).
#ifndef RS485_MSG_DRAIN_TIMEOUT_US
#define RS485_MSG_DRAIN_TIMEOUT_US  5000
#endif

// ============================================================================
// SLAVE DISCOVERY & POLLING
// ============================================================================

// Discovery scan interval (how often to probe for new slaves)
// Every N poll cycles, scan one unknown address
#ifndef RS485_DISCOVERY_INTERVAL
#define RS485_DISCOVERY_INTERVAL 50
#endif

// ============================================================================
// BUFFER SIZES
// ============================================================================

// Buffer for slave input commands (switch/encoder strings from slaves)
#ifndef RS485_INPUT_BUFFER_SIZE
#define RS485_INPUT_BUFFER_SIZE 64
#endif

// RX ring buffer size (ISR writes, loop reads)
#ifndef RS485_RX_BUFFER_SIZE
#define RS485_RX_BUFFER_SIZE    256
#endif

// ============================================================================
// FREERTOS TASK OPTIONS
// ============================================================================
//
// RS485_USE_TASK enables running RS485 master in a dedicated FreeRTOS task
// instead of being called from the main loop. This provides:
//   • Deterministic timing regardless of main loop load
//   • Consistent poll intervals (no 50-100ms jitter)
//   • Better slave response rates
//
// 1 = FreeRTOS task (recommended for production)
// 0 = Main loop (simpler, good for debugging)
//

#if RS485_USE_TASK

// Task priority (higher = more important, max typically 24)
// Should be higher than main loop to ensure consistent timing
#ifndef RS485_TASK_PRIORITY
#define RS485_TASK_PRIORITY     5
#endif

// Task stack size in bytes
#ifndef RS485_TASK_STACK_SIZE
#define RS485_TASK_STACK_SIZE   4096
#endif

// Task tick interval (how often to run poll cycle)
// 1 = every tick (~1ms), 2 = every 2 ticks, etc.
#ifndef RS485_TASK_TICK_INTERVAL
#define RS485_TASK_TICK_INTERVAL 1
#endif

// Core affinity for dual-core ESP32s (S3, classic ESP32)
// 0 = Core 0, 1 = Core 1, tskNO_AFFINITY = any core
// On single-core (S2, C3, C6), this is ignored
#ifndef RS485_TASK_CORE
#define RS485_TASK_CORE         0
#endif

// Command queue size (slave commands passed to main loop)
#ifndef RS485_CMD_QUEUE_SIZE
#define RS485_CMD_QUEUE_SIZE    16
#endif

#endif // RS485_USE_TASK

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

// Status print interval (milliseconds, 0 = disabled)
#ifndef RS485_STATUS_INTERVAL_MS
#define RS485_STATUS_INTERVAL_MS 10000
#endif

#endif // RS485_CONFIG_H
