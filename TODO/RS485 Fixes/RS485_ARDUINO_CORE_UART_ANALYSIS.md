# RS485 Arduino ESP32 Core UART Analysis — Hidden Hardware Capabilities

## Document Purpose

During development of the RS485 master driver-mode fallback, we discovered that the
ESP32 UART hardware has **native RS485 half-duplex support** that was not being used.
This document captures everything found by combing through the Arduino ESP32 core
UART implementation files. It serves as a reference for current and future RS485 work.

**Date:** February 2025
**Arduino ESP32 Core Version:** 3.3.6
**Files Analyzed:**
- `cores/esp32/HardwareSerial.h` / `.cpp`
- `cores/esp32/esp32-hal-uart.h` / `.c`
- ESP-IDF `components/esp_driver_uart/src/uart.c`
- ESP-IDF `components/hal/include/hal/uart_types.h`
- ESP-IDF `components/hal/esp32s3/include/hal/uart_ll.h`

---

## 1. Native RS485 Half-Duplex Mode

### The Discovery

The ESP32 UART peripheral has a hardware RS485 mode that automates the entire
TX/RX turnaround — DE pin management, echo removal, and bus release timing.
This is exposed through both the Arduino API and ESP-IDF.

### Available Modes

From `uart_types.h`:
```cpp
typedef enum {
    UART_MODE_UART                   = 0x00,  // Regular UART
    UART_MODE_RS485_HALF_DUPLEX      = 0x01,  // Half duplex, auto RTS/DE control
    UART_MODE_IRDA                   = 0x02,  // IrDA mode
    UART_MODE_RS485_COLLISION_DETECT = 0x03,  // RS485 with collision detection
    UART_MODE_RS485_APP_CTRL         = 0x04,  // Application-controlled RS485
} uart_mode_t;
```

### `UART_MODE_RS485_HALF_DUPLEX` — What It Does

From `uart_ll.h` (`uart_ll_set_mode_rs485_half_duplex`):
```cpp
FORCE_INLINE_ATTR void uart_ll_set_mode_rs485_half_duplex(uart_dev_t *hw)
{
    hw->conf0.sw_rts = 1;           // RTS starts HIGH (DE deasserted = RX mode)
    hw->rs485_conf.rs485tx_rx_en = 0;   // *** HARDWARE ECHO REMOVAL ***
    hw->rs485_conf.rs485rxby_tx_en = 1; // Suppress RX during TX (collision avoidance)
    hw->conf0.irda_en = 0;
    hw->rs485_conf.rs485_en = 1;        // Enable RS485 mode
}
```

**Key registers:**
- `rs485tx_rx_en = 0` — The UART hardware will NOT capture its own TX bytes
  into the RX FIFO. Echo bytes never enter the buffer. **No manual flush needed.**
- `rs485rxby_tx_en = 1` — RX is suppressed while TX is active. Prevents any
  bus contention bytes from entering the FIFO during transmission.
- `sw_rts = 1` — Software RTS control. The driver manages RTS (which is wired
  to the DE pin) automatically based on TX activity.

### Automatic DE Pin Management

From the ESP-IDF UART driver (`uart.c`), when RS485 half-duplex mode is active:

**On TX start (inside `uart_tx_all` / write path):**
```cpp
if (UART_IS_MODE_SET(uart_num, UART_MODE_RS485_HALF_DUPLEX)) {
    uart_hal_set_rts(&hal, 0);     // Assert RTS → DE HIGH → TX mode
    uart_hal_clr_intsts_mask(&hal, UART_INTR_TX_DONE);
    uart_hal_ena_intr_mask(&hal, UART_INTR_TX_DONE);
}
```

**On TX_DONE interrupt (inside driver ISR):**
```cpp
if (UART_IS_MODE_SET(uart_num, UART_MODE_RS485_HALF_DUPLEX)) {
    uart_hal_rxfifo_rst(&hal);     // Clear any stale RX data
    uart_hal_set_rts(&hal, 1);     // De-assert RTS → DE LOW → RX mode
}
```

**This is exactly what our ISR-mode TX_DONE handler does** — but it's built into
the ESP-IDF UART driver. The hardware asserts DE when TX starts and de-asserts
it the instant TX_DONE fires. Zero software timing gaps.

---

## 2. TX Buffer Modes

### Two Modes Based on `tx_buffer_size` Parameter

From `uart.h` documentation for `uart_write_bytes()`:

```
If tx_buffer_size = 0 in uart_driver_install():
  - uart_write_bytes() writes DIRECTLY to the hardware TX FIFO
  - Blocks until all data is in the FIFO (not until shifted out)
  - No ring buffer, no ISR moving bytes from buffer to FIFO
  - Simpler, more predictable, lower latency

If tx_buffer_size > 0:
  - Data goes to a software ring buffer first
  - The driver's ISR moves data from ring buffer to FIFO asynchronously
  - uart_write_bytes() returns when data is in the ring buffer (not FIFO)
  - More complex, adds scheduler-dependent latency
```

**For RS485 master:** `tx_buffer_size = 0` is ideal. Poll frames are 3 bytes,
broadcast frames are at most ~260 bytes. Both fit in the 128-byte FIFO (polls)
or are small enough for direct FIFO writes. The direct mode eliminates the
ring-buffer-to-FIFO ISR chain that was causing scheduler-dependent delays.

---

## 3. TX Completion Functions

### `uart_wait_tx_done(port, timeout)`
- Semaphore-based. Waits for the driver's TX_DONE ISR to give the semaphore.
- Returns after the ISR fires, but the actual wake-up goes through FreeRTOS
  scheduler — adds variable microseconds of delay.
- In RS485 mode, the ISR also de-asserts RTS and flushes RX before giving
  the semaphore. So by the time the caller wakes up, the bus is already released.

### `uart_wait_tx_idle_polling(port)`
- Spin-wait version. Loops on `uart_ll_is_tx_idle()` until shift register is idle.
- No scheduler involvement. Tightest possible timing.
- Used in ESP-IDF RS485 test code.
- **Cannot be used after `uart_write_bytes()` with tx_buffer_size > 0** because
  the bytes might still be in the ring buffer, not the FIFO. Would see "idle"
  prematurely. Safe with tx_buffer_size = 0 (direct FIFO mode).

### Arduino's `flush()` / `uartFlushTxOnly()`
```cpp
void uartFlushTxOnly(uart_t *uart, bool txOnly) {
    UART_MUTEX_LOCK();
    while (!uart_ll_is_tx_idle(UART_LL_GET_HW(uart->num)));
    if (!txOnly) {
        ESP_ERROR_CHECK(uart_flush_input(uart->num));
    }
    UART_MUTEX_UNLOCK();
}
```
Just polls `uart_ll_is_tx_idle()` under a mutex. Nothing special.

---

## 4. Pin Mapping

### RTS Pin = DE Pin

The ESP32 UART's RTS (Request To Send) output can be mapped to **any GPIO** using
`uart_set_pin()`. It is NOT limited to the default RTS pin. This means:

```cpp
uart_set_pin(UART_NUM_1, TX_PIN, RX_PIN, DE_PIN, UART_PIN_NO_CHANGE);
//                                         ^^^
//                                    RTS mapped to DE pin
```

The previous concern (in RS485_MASTER_TIMING_DEBUG_TRANSCRIPT.md Appendix B) that
"RTS pin is fixed to specific GPIO pins" was incorrect. Any GPIO works.

**Important:** RTS active level is ACTIVE LOW by default (standard RS232 convention).
But in RS485 half-duplex mode, the driver inverts the logic:
- `uart_hal_set_rts(0)` = RTS pin HIGH = DE asserted = TX mode
- `uart_hal_set_rts(1)` = RTS pin LOW = DE deasserted = RX mode

---

## 5. Collision Detection Mode

From `uart.c`:
```cpp
if (mode == UART_MODE_RS485_COLLISION_DETECT) {
    p_uart_obj[uart_num]->coll_det_flg = false;
    uart_hal_ena_intr_mask(&hal,
        UART_INTR_RXFIFO_TOUT | UART_INTR_RXFIFO_FULL |
        UART_INTR_RS485_CLASH | UART_INTR_RS485_FRM_ERR |
        UART_INTR_RS485_PARITY_ERR);
}
```

Available but not needed for the DCS-BIOS master. The master controls bus timing
and there should never be collisions in normal operation.

---

## 6. Arduino API Surface

### From `HardwareSerial.h`:

```cpp
bool setMode(SerialMode mode);           // Set UART_MODE_RS485_HALF_DUPLEX
bool setPins(int8_t rx, int8_t tx,
             int8_t cts, int8_t rts);     // Map RTS to DE pin
void flush(void);                         // Wait for TX complete
size_t write(const uint8_t *buf, size_t size);  // Blocking write
```

### Usage Pattern (Arduino API):
```cpp
Serial1.begin(250000, SERIAL_8N1, RX_PIN, TX_PIN);
Serial1.setPins(RX_PIN, TX_PIN, -1, DE_PIN);
Serial1.setMode(UART_MODE_RS485_HALF_DUPLEX);

// TX:
Serial1.write(data, len);
Serial1.flush();
// Hardware handles DE, echo, everything.
```

### Usage Pattern (ESP-IDF API, what we use):
```cpp
uart_driver_install(UART_NUM, 256, 0, 0, NULL, 0);  // TX buffer = 0
uart_param_config(UART_NUM, &config);
uart_set_pin(UART_NUM, TX, RX, DE_PIN, UART_PIN_NO_CHANGE);
uart_set_mode(UART_NUM, UART_MODE_RS485_HALF_DUPLEX);

// TX:
uart_write_bytes(UART_NUM, data, len);
uart_wait_tx_done(UART_NUM, timeout);
// Hardware handles DE, echo, everything.
```

---

## 7. Implications for CockpitOS RS485

### What We Were Doing (Manual Driver Mode):
1. `gpio_set_level(DE, HIGH)` — manual DE assert
2. `ets_delay_us(warmup)` — manual warmup delay
3. `uart_write_bytes()` — write through driver
4. `uart_wait_tx_done()` — wait for completion (scheduler delay)
5. `uart_flush_input()` — manual echo flush
6. `gpio_set_level(DE, LOW)` — manual DE release

### What the Hardware Can Do (RS485 Half-Duplex Mode):
1. `uart_write_bytes()` — driver asserts DE automatically via TX_DONE/RTS
2. `uart_wait_tx_done()` — by the time this returns, DE is already released,
   echo is already removed, bus is in RX mode

**6 manual steps → 2 API calls.** The hardware does the rest.

### Caveats:
- Warmup delay: The hardware RS485 mode does NOT include a pre-TX warmup.
  If the DE-pin transceiver needs settling time, we may need to keep a small
  warmup delay before `uart_write_bytes()`. Or test without it — the hardware
  may switch fast enough at 250kbaud.
- Auto-direction devices (no DE pin): Hardware RS485 mode is specifically for
  DE-pin devices. Auto-direction devices don't need it (they have no DE pin).
  For auto-direction, the simpler pattern (just write + wait + flush) works.
- RTS pin inversion: Verify that the DE pin polarity matches. Most RS485
  transceivers expect DE HIGH = transmit, which matches the driver's behavior
  in RS485 half-duplex mode.

---

*End of document.*
