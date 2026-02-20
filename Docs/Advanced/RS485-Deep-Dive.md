# RS485 Deep Dive

Deep dive into the CockpitOS RS485 bus implementation. This document covers the wire protocol, master and slave state machines, Smart Mode vs Relay Mode, ISR-driven bare-metal UART architecture, TX_DONE non-blocking transmission, slave discovery, FreeRTOS task integration, and performance tuning.

---

## 1. Architecture Overview

CockpitOS uses RS485 to connect multiple ESP32 boards on a shared half-duplex serial bus. One board is the **master** (typically the main cockpit controller), and one or more boards are **slaves** (remote panels with their own switches, encoders, and displays).

```
                    RS485 Bus (A/B differential pair)
                    ════════════════════════════════════
                    |           |           |
              ┌─────┴─────┐ ┌──┴──┐     ┌──┴──┐
              │  Master    │ │Slave│     │Slave│
              │  (ESP32)   │ │ #1  │     │ #2  │
              │            │ │     │     │     │
              │ DCS-BIOS   │ │Panel│     │Panel│
              │ USB/WiFi   │ │ I/O │     │ I/O │
              └────────────┘ └─────┘     └─────┘
```

### Master Responsibilities

- Receives DCS-BIOS export data from the PC (via USB, WiFi, or serial).
- Broadcasts export data to all slaves so they can update displays and LEDs.
- Polls each slave in round-robin order, collecting switch/encoder input commands.
- Forwards slave input commands to CockpitOS for processing (which sends them to DCS).

### Slave Responsibilities

- Receives broadcast export data and feeds it to its local DCS-BIOS parser.
- Queues input commands (switch presses, encoder turns) in a TX ring buffer.
- When polled by the master, responds with queued commands.

### Key Design Decisions

- **Bare-metal UART** -- uses `uart_ll_*` low-level hardware functions instead of the ESP-IDF UART driver for minimum latency.
- **ISR-driven** -- both RX and TX state machines run in interrupt context, matching AVR behavior for sub-microsecond response times.
- **TX_DONE non-blocking** -- TX loads the FIFO and returns immediately; the `UART_INTR_TX_DONE` interrupt handles bus turnaround.
- **100% Arduino-compatible** -- protocol matches the Arduino DCS-BIOS RS485 implementation exactly.

---

## 2. Wire Protocol

The RS485 protocol is based on the Arduino DCS-BIOS RS485 standard.

### Frame Formats

**Broadcast (master to all slaves):**
```
[Addr=0x00] [MsgType=0x00] [Length] [Data...] [Checksum]
```

**Poll (master to specific slave):**
```
[Addr=N] [MsgType=0x00] [Length=0x00]
```
Note: when `Length=0`, there is NO checksum byte. This is critical for protocol correctness.

**Slave response (with data):**
```
[Length] [MsgType=0x00] [Data...] [Checksum]
```

**Slave response (no data):**
```
[0x00]
```
A single zero byte indicates the slave has no commands to send.

### Protocol Constants

```cpp
static constexpr uint8_t ADDR_BROADCAST     = 0;     // Address 0 = broadcast to all
static constexpr uint8_t MSGTYPE_DCSBIOS    = 0;     // Message type for DCS-BIOS data
static constexpr uint8_t CHECKSUM_FIXED     = 0x72;  // Arduino-compatible fixed checksum
```

### Checksum Calculation

In Arduino-compatible mode (`RS485_ARDUINO_COMPAT=1`, the default), the checksum is always the fixed value `0x72`. The receiver intentionally ignores the checksum value -- it is present only for framing compatibility.

When Arduino compatibility is disabled, the checksum is the XOR of all data bytes (not including the header).

### Addressing

- Address `0` is reserved for broadcast -- all slaves receive it.
- Addresses `1` through `126` are valid slave addresses.
- Each slave must have a unique address configured via `RS485_SLAVE_ADDRESS`.

---

## 3. Master State Machine

### States

```cpp
enum class MasterState : uint8_t {
    IDLE,                   // Ready to send next poll or broadcast
    TX_WAITING_DONE,        // FIFO loaded, waiting for TX_DONE interrupt
    RX_WAIT_DATALENGTH,     // Waiting for first byte of slave response
    RX_WAIT_MSGTYPE,        // Waiting for message type byte
    RX_WAIT_DATA,           // Receiving data bytes
    RX_WAIT_CHECKSUM        // Waiting for checksum byte
};
```

### TX Context

The master uses a `TxContext` enum to tell the TX_DONE interrupt handler what to do after transmission completes:

```cpp
enum class TxContext : uint8_t {
    POLL,       // After TX: start listening (RX_WAIT_DATALENGTH)
    BROADCAST,  // After TX: back to IDLE
    TIMEOUT     // After TX: back to IDLE (timeout zero byte)
};
```

### State Flow

```
IDLE
  |
  +-- sendBroadcast()                    +-- sendPoll(addr)
  |     txContext = BROADCAST            |     txContext = POLL
  |     txWithEchoPrevention()           |     txWithEchoPrevention()
  |     |                                |     |
  v     v                                v     v
TX_WAITING_DONE                     TX_WAITING_DONE
  |                                      |
  | (TX_DONE ISR fires)                  | (TX_DONE ISR fires)
  |                                      |
  v                                      v
IDLE                                RX_WAIT_DATALENGTH
                                         |
                                    +----+----+
                                    |         |
                               len > 0    len == 0
                                    |         |
                                    v         v
                              RX_WAIT_     IDLE
                              MSGTYPE      (empty response)
                                    |
                                    v
                              RX_WAIT_DATA
                                    |
                                    v (all data bytes received)
                              RX_WAIT_CHECKSUM
                                    |
                                    v
                              IDLE (message complete)
```

### Timeout Handling

Two-tier timeout matching the standalone AVR master:

| Timeout | Duration | Trigger | Action |
|---------|----------|---------|--------|
| Poll timeout | `RS485_POLL_TIMEOUT_US` (1ms) | No response byte from slave | Mark slave offline, send timeout zero byte |
| Mid-message timeout | `RS485_RX_TIMEOUT_US` (5ms) | Slave started responding then went silent | Inject `\n` marker, force message complete |

---

## 4. Slave State Machine

### States

```cpp
enum class SlaveState : uint8_t {
    RX_SYNC,             // Waiting for bus silence gap (sync detection)
    RX_WAIT_ADDRESS,     // Waiting for packet address byte
    RX_WAIT_MSGTYPE,     // Waiting for message type byte
    RX_WAIT_LENGTH,      // Waiting for data length byte
    RX_WAIT_DATA,        // Receiving data bytes
    RX_WAIT_CHECKSUM,    // Waiting for checksum byte
    RX_SKIP_LENGTH,      // Skipping another slave's response (length byte)
    RX_SKIP_DATA,        // Skipping another slave's response (data bytes)
    TX_WAITING_DONE      // FIFO loaded with response, waiting for TX_DONE
};
```

### Sync Detection

The slave uses a gap-based sync mechanism. If no byte arrives for `RS485_SYNC_TIMEOUT_US` (500us default), the slave assumes the next byte starts a new packet:

```cpp
// In ISR context:
if (state == SlaveState::RX_SYNC) {
    if ((now - lastRxTime) >= SYNC_TIMEOUT_US) {
        state = SlaveState::RX_WAIT_ADDRESS;
        // Fall through to process this byte as address
    } else {
        lastRxTime = now;
        continue;  // Stay in sync, discard byte
    }
}
```

This approach is robust because:
- At 250kbps, inter-byte gap within a packet is ~40us.
- The 500us threshold is well above intra-packet gaps but below the gap between master poll cycles.

### Packet Routing

When a complete packet is received, the slave routes it based on the address:

| Address | Action |
|---------|--------|
| `ADDR_BROADCAST` (0) | Buffer export data for the DCS-BIOS parser |
| `RS485_SLAVE_ADDRESS` (our address) | Send response immediately via `sendResponseISR()` |
| Any other address | Enter skip states to consume the other slave's response |

### Skip States

When the master polls a different slave, our slave must consume that slave's response to stay synchronized. The skip logic:

1. `RX_SKIP_LENGTH` -- read the response length byte. If `0`, no more bytes to skip. Otherwise, set `skipRemaining = length + 2` (data bytes + MsgType + checksum).
2. `RX_SKIP_DATA` -- decrement `skipRemaining` for each byte until zero, then return to `RX_WAIT_ADDRESS`.

---

## 5. TX_DONE Non-Blocking Pattern

This is the most critical architectural detail in both the master and slave. The pattern eliminates blocking delays during transmission.

### The Problem (v2.2 and earlier)

Early versions used a blocking approach: assert DE (direction enable), send bytes with spin-wait, delay for cooldown, deassert DE. This held the bus for ~52us after the last byte, creating a window where fast auto-direction slaves could respond while the master's DE pin was still HIGH, causing bus contention.

### The Solution (v2.3+)

TX uses the hardware `UART_INTR_TX_DONE` interrupt, which fires when the last bit of the last byte has physically left the UART shift register:

```cpp
// TRANSMIT (non-blocking):
static void txWithEchoPrevention(const uint8_t* data, size_t len) {
    disableRxInt();                          // Prevent echo bytes
    deAssert();                              // DE HIGH (TX mode)

    // CRITICAL ORDERING: state -> clear -> FIFO -> enable
    state = MasterState::TX_WAITING_DONE;    // 1. Set state FIRST
    uart_ll_clr_intsts_mask(..., TX_DONE);   // 2. Clear stale TX_DONE
    uart_ll_write_txfifo(..., data, len);    // 3. Load FIFO (TX starts)
    uart_ll_ena_intr_mask(..., TX_DONE);     // 4. Enable TX_DONE interrupt

    // Return immediately -- CPU is FREE while hardware shifts out bytes
}

// TX_DONE ISR (fires when last bit leaves shift register):
if (uart_intr_status & UART_INTR_TX_DONE) {
    flushRxFifo();              // Flush echo bytes
    deDeassert();               // DE LOW (RX mode) -- bus released
    enableRxInt();              // Resume listening
    uart_ll_disable_intr_mask(..., TX_DONE);  // One-shot

    // Transition based on context
    if (txContext == TxContext::POLL) {
        rxStartTime = esp_timer_get_time();
        state = MasterState::RX_WAIT_DATALENGTH;
    } else {
        state = MasterState::IDLE;
    }
}
```

### Critical Ordering

The `state -> clear -> FIFO -> enable` ordering prevents a race condition on ESP32-S2 (single-core):

1. **Set state FIRST** -- the TX_DONE ISR must see `TX_WAITING_DONE` before it can fire.
2. **Clear stale TX_DONE** -- removes any leftover interrupt status from the previous cycle.
3. **Load FIFO** -- hardware begins transmitting immediately.
4. **Enable TX_DONE LAST** -- even if the UART finishes TX between step 3 and step 4, the `int_raw` bit accumulates. When `int_ena` is set, `int_st = int_raw AND int_ena` fires the ISR immediately.

The old ordering (FIFO -> state -> clear -> enable) had a fatal race where tick preemption between FIFO load and clear could erase the TX_DONE event, leaving the state machine stuck.

---

## 6. Smart Mode vs Relay Mode

The master supports two fundamentally different approaches to broadcasting export data to slaves.

### Smart Mode (`RS485_SMART_MODE=1`)

Smart Mode parses the DCS-BIOS export stream, filters by address, detects changes, and sends only relevant delta updates.

```
DCS-BIOS Export Stream (from PC)
   |
   v
parseExportByte()
   |
   +-- Sync detection (4x 0x55 bytes)
   +-- Extract address/value pairs
   |
   v
processAddressValue(address, value)
   |
   +-- Filter: findDcsOutputEntries(address)
   |     Discard addresses not in DcsOutputTable
   |
   +-- Change detect (optional): prevExport[address] == value?
   |     Skip if unchanged
   |
   v
queueChange(address, value)
   |
   v
Change Queue (ring buffer, RS485_CHANGE_QUEUE_SIZE entries)
   |
   v
prepareBroadcastData()
   |
   +-- Reconstruct valid DCS-BIOS frames for each change
   |     [0x55 0x55 0x55 0x55] [addr_lo addr_hi] [0x02 0x00] [val_lo val_hi]
   |
   v
sendBroadcast()
   |
   +-- Frame: [Addr=0][MsgType=0][Length][Reconstructed frames...][Checksum]
```

**Advantages:**
- Bandwidth reduction of 100-1000x (only changed, relevant addresses are sent).
- Slaves receive only data they need.
- Works with the existing DCS-BIOS parser on the slave side.

**Cost:**
- 32KB RAM for `prevExport[]` when `RS485_CHANGE_DETECT=1` (optional).
- Change queue uses `RS485_CHANGE_QUEUE_SIZE * 4` bytes.

### Relay Mode (`RS485_SMART_MODE=0`)

Relay Mode acts as a transparent byte pipe, forwarding the raw DCS-BIOS export stream without any parsing or filtering.

```
DCS-BIOS Export Stream (from PC)
   |
   v
bufferRawByte()
   |
   v
Raw Ring Buffer (RS485_RAW_BUFFER_SIZE bytes, 512 default)
   |
   v
prepareBroadcastData()
   |
   +-- Drain up to RS485_RELAY_CHUNK_SIZE bytes (124 default)
   |
   v
sendBroadcast()
   |
   +-- Frame: [Addr=0][MsgType=0][Length][Raw bytes...][Checksum]
```

**Advantages:**
- Works with any aircraft, any address -- no DcsOutputTable dependency.
- Minimal RAM usage (512 bytes).
- Ideal for debugging or when panel configurations are unknown.

**Cost:**
- Full bandwidth -- every byte is forwarded regardless of relevance.
- Slaves must parse and filter the entire export stream themselves.

---

## 7. Slave Discovery & Polling

### Discovery Logic

The master maintains a `slavePresent[128]` boolean array tracking which slave addresses have responded to polls.

Discovery uses a periodic scan:

```cpp
static uint8_t advancePollAddress() {
    discoveryCounter++;
    if (discoveryCounter >= RS485_DISCOVERY_INTERVAL) {
        discoveryCounter = 0;
        // Probe the next unknown address
        for (uint8_t i = 1; i <= RS485_MAX_SLAVE_ADDRESS; i++) {
            probeAddr = (probeAddr % RS485_MAX_SLAVE_ADDRESS) + 1;
            if (!slavePresent[probeAddr]) return probeAddr;
        }
    }

    // Normal operation: round-robin through known slaves
    for (...) {
        if (slavePresent[currentPollAddr]) return currentPollAddr;
    }

    // No known slaves: scan sequentially
    return nextSequentialAddress;
}
```

Every `RS485_DISCOVERY_INTERVAL` (50 default) poll cycles, the master probes one unknown address. This means new slaves are discovered within seconds of being connected, without flooding the bus with empty polls.

### Slave Offline Detection

When a poll times out (`RS485_POLL_TIMEOUT_US` = 1ms with no response), the master marks that slave as offline:

```cpp
slavePresent[currentPollAddr] = false;
```

The slave will be re-discovered during the next discovery scan cycle.

### Poll Priority

The main poll loop prioritizes broadcasts over polls, but ensures polls happen at least every `RS485_MAX_POLL_INTERVAL_US` (2ms):

```
if (hasBroadcastData && timeSinceLastPoll < MAX_POLL_INTERVAL) {
    sendBroadcast();    // Priority 1: broadcast if data available
} else {
    sendPoll(nextAddr); // Priority 2: poll a slave
}
```

---

## 8. Slave Response Path

### Command Queuing

The slave queues input commands in a TX ring buffer:

```cpp
bool RS485Slave_queueCommand(const char* label, const char* value) {
    size_t needed = strlen(label) + 1 + strlen(value) + 1;  // "LABEL VALUE\n"

    if (needed > RS485_TX_BUFFER_SIZE - txCount()) {
        statTxDrops++;
        return false;  // Buffer full
    }

    portDISABLE_INTERRUPTS();
    // Write "LABEL VALUE\n" into ring buffer
    portENABLE_INTERRUPTS();
    return true;
}
```

Commands are formatted as `"LABEL VALUE\n"` strings, matching the Arduino DCS-BIOS protocol exactly. The critical section (`portDISABLE_INTERRUPTS`) protects the ring buffer from concurrent ISR access.

### ISR Response

When the slave is polled (address matches, MsgType == DCSBIOS), it sends the response directly from the ISR:

```cpp
// In ISR context:
if (packetAddr == RS485_SLAVE_ADDRESS) {
    sendResponseISR();   // Load FIFO + arm TX_DONE, return immediately
    goto done_rx;        // Exit RX loop (echo bytes will be flushed by TX_DONE)
}
```

The `sendResponseISR()` function builds the complete response frame in a static buffer (`txFrameBuf`), writes it to the UART FIFO in one burst, and arms the TX_DONE interrupt. The ISR cost is O(1) regardless of response size -- the hardware shifts out bytes autonomously.

### Response Format

```cpp
// Empty response (no commands queued):
[0x00]                    // Single byte

// Response with commands:
[Length] [MsgType=0x00] [Data...] [Checksum=0x72]

// Example: "MASTER_ARM_SW 1\n"
[0x10] [0x00] [M][A][S][T][E][R][_][A][R][M][_][S][W][ ][1][\n] [0x72]
```

---

## 9. RISC-V Memory Barriers

ESP32-C3, C6, and H2 use RISC-V cores where cache coherency is not guaranteed between ISR and task contexts. CockpitOS uses explicit memory fences:

```cpp
// Master: after reading FIFO byte
#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C6 || CONFIG_IDF_TARGET_ESP32H2
    #define RS485_RISCV_FENCE() __asm__ __volatile__("fence" ::: "memory")
#else
    #define RS485_RISCV_FENCE() do {} while(0)
#endif

// Slave: before reading txBuffer in ISR (written by task context)
#ifdef __riscv
    __asm__ __volatile__("fence" ::: "memory");
#endif
```

Two specific cases require fences:

1. **FIFO read coherency** -- after `uart_ll_read_rxfifo()`, the FIFO pointer update must propagate before the next length check. Without this, the CPU can read stale/duplicate bytes.
2. **txBuffer coherency** -- the slave's `queueCommand()` writes to `txBuffer` from task context, and `sendResponseISR()` reads it from ISR context. The fence ensures task-context writes are visible to the ISR.

Xtensa cores (ESP32, ESP32-S2, ESP32-S3) have implicit barrier semantics from `RSIL` (the interrupt disable instruction), so no explicit fence is needed.

---

## 10. FreeRTOS Task Integration

Both master and slave can optionally run in dedicated FreeRTOS tasks instead of being called from the main loop.

### When to Use Task Mode

- **Recommended for production** -- provides deterministic timing regardless of main loop load.
- **Critical for single-core chips** (ESP32-S2, C3, C6) -- the main loop may block on WiFi or USB operations, causing missed polls.
- **Always use for master** -- the master controls bus timing and must poll consistently.

### Task Architecture

```cpp
// Master task
static void rs485Task(void* param) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    while (taskRunning) {
        rs485PollLoop();
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(RS485_TASK_TICK_INTERVAL));
    }
    vTaskDelete(nullptr);
}
```

The task uses `vTaskDelayUntil()` for precise 1ms tick intervals. The poll loop itself takes ~5-10us of CPU time, sleeping for the remainder of each tick.

### Command Queue (Master)

In task mode, the master cannot call `sendCommand()` directly from the task (it must run on the main loop). Instead, slave commands are queued:

```cpp
struct SlaveCommand { char label[48]; char value[16]; };

static QueueHandle_t cmdQueue;

// In task context:
queueSlaveCommand(label, value);   // Non-blocking xQueueSend

// In main loop:
void RS485Master_loop() {
    processQueuedCommands();       // Drain queue via xQueueReceive
}
```

### Priority Considerations

The master task runs at priority `RS485_TASK_PRIORITY` (24 default) to ensure it gets CPU time even when WiFi (23), USB (18), and other system tasks are active. It uses `vTaskDelayUntil()` which yields immediately after completing work, consuming less than 1% of CPU.

### Core Pinning

On dual-core chips (ESP32, ESP32-S3), the task is pinned to a specific core:

```cpp
// Dual-core: use xTaskCreatePinnedToCore
xTaskCreatePinnedToCore(rs485Task, "RS485M",
    RS485_TASK_STACK_SIZE,    // 4096 bytes
    nullptr,
    RS485_TASK_PRIORITY,      // 24
    &rs485TaskHandle,
    RS485_TASK_CORE);         // 0 (default)

// Single-core (S2, C3, C6): use xTaskCreate (no affinity)
xTaskCreate(rs485Task, "RS485M", ...);
```

---

## 11. Configuration Reference

### Master Configuration (RS485Config.h)

| Define | Default | Description |
|--------|---------|-------------|
| `RS485_MASTER_ENABLED` | 0 | Enable master mode |
| `RS485_SMART_MODE` | 0 | 1=Smart (filtered), 0=Relay (raw bytes) |
| `RS485_CHANGE_DETECT` | 0 | Enable delta compression (Smart Mode only) |
| `RS485_BAUD` | 250000 | Bus baud rate |
| `RS485_UART_NUM` | 1 | UART peripheral number |
| `RS485_POLL_TIMEOUT_US` | 1000 | First-byte response timeout |
| `RS485_RX_TIMEOUT_US` | 5000 | Mid-message timeout |
| `RS485_MAX_POLL_INTERVAL_US` | 2000 | Max time between polls |
| `RS485_DISCOVERY_INTERVAL` | 50 | Poll cycles between discovery scans |
| `RS485_CHANGE_QUEUE_SIZE` | 128 | Change queue entries (Smart Mode) |
| `RS485_MAX_BROADCAST_CHUNK` | 64 | Max bytes per broadcast |
| `RS485_RAW_BUFFER_SIZE` | 512 | Raw buffer size (Relay Mode) |
| `RS485_RELAY_CHUNK_SIZE` | 124 | Max bytes per relay broadcast |
| `RS485_INPUT_BUFFER_SIZE` | 256 | Slave command input buffer |
| `RS485_TX_WARMUP_DELAY_US` | 0 | DE pin warmup delay |
| `RS485_MSG_DRAIN_TIMEOUT_US` | 5000 | Force-clear stalled messages |
| `RS485_USE_TASK` | 1 | Run in FreeRTOS task |
| `RS485_TASK_PRIORITY` | 24 | Task priority |
| `RS485_TASK_STACK_SIZE` | 4096 | Task stack bytes |
| `RS485_TASK_CORE` | 0 | Core affinity (dual-core only) |

### Slave Configuration (RS485SlaveConfig.h)

| Define | Default | Description |
|--------|---------|-------------|
| `RS485_SLAVE_ENABLED` | 0 | Enable slave mode |
| `RS485_SLAVE_ADDRESS` | (required) | Unique address 1-126 |
| `RS485_BAUD` | 250000 | Bus baud rate (must match master) |
| `RS485_UART_NUM` | 1 | UART peripheral number |
| `RS485_ARDUINO_COMPAT` | 1 | Use fixed 0x72 checksum |
| `RS485_TX_BUFFER_SIZE` | 128 | Outgoing command buffer |
| `RS485_EXPORT_BUFFER_SIZE` | 512 | Incoming export data buffer |
| `RS485_SYNC_TIMEOUT_US` | 500 | Bus silence gap for sync detection |
| `RS485_TX_PRE_DE_DELAY_US` | 40 | Pre-DE silent gap (AVR compat) |
| `RS485_USE_TASK` | 1 | Run in FreeRTOS task |
| `RS485_TASK_PRIORITY` | 5 | Task priority |
| `RS485_TASK_STACK_SIZE` | 4096 | Task stack bytes |
| `RS485_TASK_CORE` | 0 | Core affinity (dual-core only) |

---

## 12. Statistics & Debugging

### Master Statistics

The master tracks several counters:

| Counter | Meaning |
|---------|---------|
| `statPolls` | Total polls sent |
| `statPollsPresent` | Polls to known-present slaves |
| `statResponses` | Valid responses received |
| `statMidMsgTimeouts` | Slave started responding then went silent |
| `statDrainStalls` | messageBuffer stuck (task starvation) |
| `statBroadcasts` | Broadcast frames sent |
| `statInputCmds` | Input commands forwarded from slaves |
| `statBytesOut` | Total bytes transmitted |
| `messageBuffer.dropCount` | Bytes dropped due to buffer overflow |

Enable periodic logging with `RS485_STATUS_INTERVAL_MS` (60000ms default):

```
[RS485M] Polls=1250(1200) Resp=99.2% Bcasts=450 Cmds=23 Slaves=2 Queue=0 Drops=0 MidT=0 Drain=0
```

### Slave Statistics

| Counter | Meaning |
|---------|---------|
| `statPolls` | Polls received (addressed to us) |
| `statBroadcasts` | Broadcast packets received |
| `statExportBytes` | Export bytes fed to DCS-BIOS parser |
| `statCommandsSent` | Responses sent with command data |
| `statTxDrops` | Commands dropped (TX buffer full) |

### Print Status

Call `RS485Master_printStatus()` or `RS485Slave_printStatus()` to get a complete status dump including online slave list, mode, counters, and task information.

---

## 13. Hardware Options

### Option 1: Built-in RS485 Transceiver

Boards like the Waveshare ESP32-S3-RS485-CAN have an onboard RS485 transceiver with manual DE pin control.

```cpp
// Config.h
#define RS485_TX_PIN    17
#define RS485_RX_PIN    18
#define RS485_DE_PIN    21   // Manual direction control
```

### Option 2: External MAX485

Connect an external MAX485 module:

```
ESP32 TX  -> MAX485 DI
ESP32 RX  <- MAX485 RO
ESP32 GPIO -> MAX485 DE + RE (tied together)
```

### Option 3: Auto-Direction Module

Auto-direction RS485 modules handle TX/RX switching automatically. No DE pin needed:

```cpp
#define RS485_DE_PIN    -1   // Auto-direction mode
```

When `RS485_DE_PIN` is -1, all DE-related code is compiled out via `#if RS485_DE_PIN >= 0` guards. The warmup and cooldown delays are also skipped for auto-direction devices.

---

## See Also

- [Reference/Config.md](../Reference/Config.md) -- RS485 configuration constants
- [Advanced/FreeRTOS-Tasks.md](FreeRTOS-Tasks.md) -- Task management details
- [Advanced/Custom-Panels.md](Custom-Panels.md) -- Panel architecture
