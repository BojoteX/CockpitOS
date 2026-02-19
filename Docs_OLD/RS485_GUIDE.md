# RS485 Multi-Panel Networking Guide

CockpitOS supports RS485 multi-drop networking to connect multiple ESP32 panels over a single twisted-pair bus. One ESP32 acts as the **Master** (connected to DCS World via WiFi, USB, Serial, or Bluetooth), and one or more ESP32s act as **Slaves** (connected only via the RS485 bus).

This guide covers what RS485 is, why you would use it, how to set up both masters and slaves, configuration options, interoperability with existing Arduino DCS-BIOS RS485 setups, and advanced topologies that combine RS485 with CockpitOS's other transport modes.

---

## Table of Contents

1. [What is RS485 and Why Use It?](#what-is-rs485-and-why-use-it)
2. [Architecture Overview](#architecture-overview)
3. [Hardware Requirements](#hardware-requirements)
4. [Setting Up the Master](#setting-up-the-master)
5. [Setting Up Slaves](#setting-up-slaves)
6. [Configuration Reference](#configuration-reference)
7. [Smart Mode vs Relay Mode](#smart-mode-vs-relay-mode)
8. [Interoperability with Arduino DCS-BIOS](#interoperability-with-arduino-dcsbios)
9. [Advanced Topologies](#advanced-topologies)
10. [Debugging and Diagnostics](#debugging-and-diagnostics)
11. [Performance and Limits](#performance-and-limits)
12. [Troubleshooting](#troubleshooting)

---

## What is RS485 and Why Use It?

RS485 is a differential signaling standard designed for long-distance, noise-immune multi-drop communication. A single two-wire bus (A/B twisted pair) can connect up to 32 devices over distances of 1200 meters.

**Why RS485 for cockpit panels?**

- **Single cable for many panels**: Run one twisted pair through your cockpit frame and connect dozens of panels to it. No need for individual USB or network cables per panel.
- **Noise immunity**: Differential signaling rejects electrical noise from servos, backlighting, and other cockpit electronics. This matters when your panels share a power supply with motors and LEDs.
- **Long distances**: RS485 works reliably up to 1200m at low speeds and comfortably handles 250kbaud at 15-30 meters. Your cockpit wiring is well within limits.
- **Proven protocol**: The DCS-BIOS RS485 protocol has been used by the Arduino DCS-BIOS community for years. CockpitOS implements the same protocol, so existing knowledge and wiring applies.
- **Cost effective**: ESP32 dev boards cost a few dollars each, and RS485 transceiver modules (MAX485, SP3485) cost under a dollar. A full multi-panel setup costs a fraction of equivalent USB or network infrastructure.

**When NOT to use RS485:**

- If you only have one panel, use WiFi or USB directly.
- If you need TFT displays on every panel, RS485 bandwidth may be tight. Consider WiFi for display-heavy panels and RS485 for simple switch/LED panels.
- If latency under 1ms is critical for a specific control, connect that panel via USB and use RS485 for the rest.

---

## Architecture Overview

```
                         DCS World PC
                              |
                         WiFi / USB / Serial
                              |
                    [ ESP32 - CockpitOS MASTER ]
                         RS485_MASTER_ENABLED=1
                         USE_DCSBIOS_WIFI=1 (or USB, etc.)
                              |
                    RS485 Bus (A/B twisted pair)
                    __________|__________
                   |          |          |
              [ SLAVE 1 ] [ SLAVE 2 ] [ SLAVE 3 ]
              Addr=1       Addr=2       Addr=3
              RS485_SLAVE_ENABLED=1
```

**Data flow:**

1. **DCS World to Slaves (Export)**: DCS World sends cockpit state to the master via its chosen transport (WiFi, USB, etc.). The master broadcasts this data on the RS485 bus. All slaves receive it and update their LEDs, displays, and gauges.

2. **Slaves to DCS World (Input)**: When a pilot flips a switch on a slave panel, the slave queues the command. The master polls each slave in round-robin order. When polled, the slave sends its pending commands. The master forwards them to DCS World.

**The master is both a CockpitOS panel AND an RS485 coordinator.** It has its own inputs, LEDs, and displays while also managing the RS485 bus. The `RS485_MASTER_ENABLED` flag is an overlay on top of a normal transport, not a replacement.

---

## Hardware Requirements

### RS485 Transceiver Options

Each ESP32 on the bus needs an RS485 transceiver. Three options:

| Option | Example | DE Pin Needed | Notes |
|--------|---------|---------------|-------|
| Built-in transceiver | Waveshare ESP32-S3-RS485-CAN | Yes (usually GPIO21) | Easiest, no external wiring |
| External MAX485 | MAX485, SP3485, SN65HVD75 | Yes (user-chosen GPIO) | Most common, very cheap |
| Auto-direction module | Many eBay/AliExpress modules | No (set DE_PIN = -1) | Module handles TX/RX switching |

### Wiring

For external MAX485 modules:

```
ESP32 TX pin  -->  MAX485 DI  (Data In)
ESP32 RX pin  <--  MAX485 RO  (Receiver Out)
ESP32 DE pin  -->  MAX485 DE + RE (tied together)

MAX485 A  ----  bus A wire (twisted pair)
MAX485 B  ----  bus B wire (twisted pair)
MAX485 VCC --- 3.3V or 5V (check your module)
MAX485 GND --- GND (common ground for all devices!)
```

**Critical wiring rules:**

- **Common ground**: ALL devices on the bus MUST share a ground connection. Without it, differential signaling fails and you get garbage data.
- **Twisted pair**: Use twisted pair cable for A/B lines. Cat5/Cat6 ethernet cable works perfectly (use one pair).
- **Termination**: For runs over 5 meters, add 120-ohm termination resistors across A/B at both ends of the bus. For short runs in a cockpit, this is usually unnecessary.
- **Bus topology**: Use a daisy-chain (linear bus), not a star. Each device taps off the main cable run.

### Pin Defaults

The default pins in `Config.h` work with the Waveshare ESP32-S3-RS485-CAN board:

```
RS485_TX_PIN = 17
RS485_RX_PIN = 18
RS485_DE_PIN = 21    (set to -1 for auto-direction modules)
```

Change these to match your wiring. Any available GPIO pins work.

---

## Setting Up the Master

The master is a normal CockpitOS panel that also coordinates the RS485 bus.

### Step 1: Choose a Transport

The master needs a connection to DCS World. Pick one:

```c
// Config.h - Master configuration
#define USE_DCSBIOS_WIFI     1    // WiFi is recommended for master
#define USE_DCSBIOS_USB      0
#define USE_DCSBIOS_SERIAL   0
```

WiFi is recommended for the master because it doesn't tie up a USB port and works wirelessly. USB gives lowest latency if you need it.

### Step 2: Enable Master Mode

```c
// Config.h
#define RS485_MASTER_ENABLED    1    // Enable RS485 master
#define RS485_SLAVE_ENABLED     0    // Must be 0 (mutually exclusive)
```

### Step 3: Configure RS485 Pins

```c
// Config.h
#define RS485_TX_PIN            17   // Your TX GPIO
#define RS485_RX_PIN            18   // Your RX GPIO
#define RS485_DE_PIN            21   // Your DE GPIO, or -1 for auto-direction
```

### Step 4: Set Polling Range

```c
// Config.h
#define RS485_MAX_SLAVE_ADDRESS  32  // Poll addresses 1 through 32
```

Set this to the highest slave address you use. Lower values reduce discovery time. The master automatically discovers which addresses have active slaves.

### Step 5: Choose Operating Mode

```c
// Config.h
#define RS485_SMART_MODE        0    // 0 = Relay, 1 = Smart (see section below)
```

**Start with Relay Mode (0)** for initial setup and debugging. Switch to Smart Mode (1) once your panel configuration is stable.

### Step 6: Flash and Verify

Flash the master with Arduino IDE. Open the debug console (WiFi UDP or Serial). You should see:

```
[RS485M] Init OK: 250000 baud, TX=17, RX=18, DE=21
[RS485M] Mode: RELAY (raw bytes, Arduino-compatible)
[RS485M] Task created: priority=5, stack=4096
```

The master will immediately start scanning for slaves.

---

## Setting Up Slaves

Each slave is an independent CockpitOS install with its own label set, inputs, and outputs.

### Step 1: Enable Slave Mode

```c
// Config.h
#define RS485_SLAVE_ENABLED     1    // This IS the transport
#define RS485_SLAVE_ADDRESS     1    // Unique address 1-126
#define RS485_MASTER_ENABLED    0    // Must be 0

// Disable all other transports (slave IS a transport)
#define USE_DCSBIOS_WIFI        0
#define USE_DCSBIOS_USB         0
#define USE_DCSBIOS_SERIAL      0
#define USE_DCSBIOS_BLUETOOTH   0
```

**Every slave must have a unique address.** Address 0 is reserved for broadcast. Valid range is 1-126.

### Step 2: Configure RS485 Pins

Same as the master:

```c
#define RS485_TX_PIN            17
#define RS485_RX_PIN            18
#define RS485_DE_PIN            21   // or -1
```

### Step 3: Create a Label Set

Each slave needs its own label set matching the panels it physically hosts. Follow the standard label set workflow:

1. Copy an existing label set: `cp -r LABEL_SET_IFEI LABEL_SET_SLAVE1`
2. Edit `selected_panels.txt` to enable only this slave's panels
3. Run `python generate_data.py`
4. Edit `InputMapping.h` and `LEDMapping.h` for your physical wiring

### Step 4: Flash and Verify

```
[RS485S] SLAVE INITIALIZED (ISR Mode)
[RS485S]   Address: 1
[RS485S]   Baud: 250000
[RS485S]   RX: ISR-driven (bare-metal UART, FIFO threshold=1)
```

Once the master detects this slave, you will see periodic status:

```
[RS485S] Polls=150 Bcasts=30 Export=7680 Cmds=2 TxPend=0
```

---

## Configuration Reference

### Master Settings (`Config.h` + `RS485Config.h`)

| Define | Default | Description |
|--------|---------|-------------|
| `RS485_MASTER_ENABLED` | 0 | Enable master mode |
| `RS485_SMART_MODE` | 0 | 0=Relay, 1=Smart filtered broadcasting |
| `RS485_MAX_SLAVE_ADDRESS` | 32 | Highest address to poll (1-127) |
| `RS485_TX_PIN` | 17 | UART TX GPIO |
| `RS485_RX_PIN` | 18 | UART RX GPIO |
| `RS485_DE_PIN` | 21 | Direction Enable GPIO, -1 for auto |
| `RS485_BAUD` | 250000 | Bus speed (must match all devices) |
| `RS485_POLL_TIMEOUT_US` | 1000 | Wait for first response byte (us) |
| `RS485_RX_TIMEOUT_US` | 5000 | Wait for message completion (us) |
| `RS485_MAX_POLL_INTERVAL_US` | 2000 | Max time between polls (us) |
| `RS485_DISCOVERY_INTERVAL` | 50 | Probe unknown address every N cycles |
| `RS485_USE_TASK` | 1 | Run in dedicated FreeRTOS task |
| `RS485_TASK_PRIORITY` | 5 | FreeRTOS task priority |
| `RS485_CHANGE_QUEUE_SIZE` | 128 | Smart mode change queue (entries) |
| `RS485_RELAY_CHUNK_SIZE` | 128 | Relay mode bytes per broadcast |
| `RS485_STATUS_INTERVAL_MS` | 10000 | Status log interval (0=disabled) |

### Slave Settings (`Config.h` + `RS485SlaveConfig.h`)

| Define | Default | Description |
|--------|---------|-------------|
| `RS485_SLAVE_ENABLED` | 0 | Enable slave mode |
| `RS485_SLAVE_ADDRESS` | 1 | Unique bus address (1-126) |
| `RS485_TX_PIN` | 17 | UART TX GPIO |
| `RS485_RX_PIN` | 18 | UART RX GPIO |
| `RS485_DE_PIN` | 21 | Direction Enable GPIO, -1 for auto |
| `RS485_BAUD` | 250000 | Must match master |
| `RS485_USE_ISR_MODE` | 1 | 1=ISR-driven (recommended), 0=driver fallback |
| `RS485_ARDUINO_COMPAT` | 1 | 1=Fixed 0x72 checksum (AVR compatible) |
| `RS485_TX_BUFFER_SIZE` | 128 | Outgoing command buffer (bytes) |
| `RS485_EXPORT_BUFFER_SIZE` | 512 | Incoming export buffer (bytes) |
| `RS485_USE_TASK` | 1 | Run in dedicated FreeRTOS task |
| `RS485_TASK_PRIORITY` | 5 | FreeRTOS task priority |
| `RS485_STATUS_INTERVAL_MS` | 5000 | Status log interval (0=disabled) |

---

## Smart Mode vs Relay Mode

The master has two operating modes that control how it broadcasts DCS-BIOS export data to slaves.

### Relay Mode (`RS485_SMART_MODE=0`)

**How it works**: The master receives the raw DCS-BIOS export byte stream and forwards it byte-for-byte to the RS485 bus, chunked into RS485 packets.

**Pros**:
- Zero parsing overhead
- Works with ANY aircraft, ANY DCS-BIOS version
- Only 512 bytes of RAM
- Perfect for debugging since you see the exact same data DCS sends

**Cons**:
- Broadcasts ALL cockpit data, even addresses no slave needs
- No change detection: broadcasts the full stream every frame (~30Hz)
- Higher bus utilization

**Use when**: Getting started, debugging, or when you have few slaves and don't need bandwidth optimization.

### Smart Mode (`RS485_SMART_MODE=1`)

**How it works**: The master parses the DCS-BIOS export stream, filters it against the `DcsOutputTable` (generated from your label sets), and only broadcasts addresses that your slaves actually use. Additionally, with `RS485_CHANGE_DETECT=1` (default), it tracks previous values and only broadcasts changed data.

**Pros**:
- 100-1000x bandwidth reduction in typical use
- Only sends addresses your panels care about
- Change detection eliminates redundant broadcasts
- Frees bus time for faster slave polling

**Cons**:
- 32KB RAM for change tracking
- Requires accurate label sets (slaves won't receive unlisted addresses)
- Must regenerate data if panel configuration changes

**Use when**: Production deployment with known panel configurations and more than 2-3 slaves.

**Important**: Smart Mode filters based on `DcsOutputTable` generated by `generate_data.py`. If a slave needs an address that isn't in the master's label set, the slave won't receive updates for it. Make sure the master's label set includes ALL panels across ALL slaves.

---

## Interoperability with Arduino DCS-BIOS

CockpitOS RS485 is 100% wire-compatible with the Arduino DCS-BIOS RS485 implementation (`DcsBiosNgRS485Master` and `DcsBiosNgRS485Slave`).

### CockpitOS Master + Arduino Slaves

Works out of the box in **Relay Mode** (`RS485_SMART_MODE=0`). The master forwards the raw DCS-BIOS stream, which is exactly what Arduino slaves expect. Arduino slaves respond with the standard RS485 response format, which the CockpitOS master parses correctly.

**Smart Mode** also works because the master reconstructs valid DCS-BIOS frames from the change queue. Each change becomes a complete frame with sync header, address, count, and data. Arduino slaves parse these just like the original export stream.

### Arduino Master + CockpitOS Slaves

Works out of the box. The CockpitOS slave speaks the same protocol. Ensure:
- `RS485_ARDUINO_COMPAT=1` (default) so the slave uses the fixed 0x72 checksum that the Arduino master expects.
- Baud rate matches (Arduino default is also 250000).

### Mixed Bus

You can mix CockpitOS and Arduino slaves on the same bus under any master. Each device only needs to speak the same RS485 protocol, which they all do.

### What CockpitOS Adds Over Arduino

| Feature | Arduino RS485 | CockpitOS RS485 |
|---------|--------------|-----------------|
| Master transport | USB Serial only | WiFi, USB HID, Serial, Bluetooth |
| Smart filtering | No | Yes (DcsOutputTable + change detection) |
| Bus diagnostics | None | Poll count, response rate, timeouts, export bytes, status logging |
| Debug logging | Serial only | WiFi UDP debug (works alongside RS485!) |
| ISR-driven slave | Yes (AVR native) | Yes (ESP32 bare-metal UART ISR) |
| Hot-plug detection | Manual | Automatic discovery scan |
| Multi-transport master | No | Master can serve RS485 + run its own WiFi/USB panels |
| FreeRTOS task isolation | No | Dedicated task with configurable priority/core |

---

## Advanced Topologies

CockpitOS's transport flexibility enables topologies that aren't possible with Arduino alone.

### Topology 1: WiFi Master + RS485 Slaves (Recommended)

```
PC (DCS World) <-- WiFi --> [Master ESP32] <-- RS485 --> [Slave 1] [Slave 2] ...
```

The master connects to DCS via WiFi and broadcasts to slaves over RS485. No USB cables at all. Power each ESP32 from a common PSU.

**Advantages**: Completely wireless connection to PC. One cable run (RS485 + power) through the cockpit.

### Topology 2: USB Master + RS485 Slaves (Lowest Latency)

```
PC (DCS World) <-- USB HID --> [Master ESP32-S3] <-- RS485 --> [Slave 1] [Slave 2] ...
```

The master uses USB HID for minimum latency to DCS, and distributes to slaves via RS485.

**Advantages**: Fastest possible PC-to-master link. Good for critical controls (gear, weapons) on the master panel.

### Topology 3: Multiple RS485 Buses

```
PC <-- WiFi --> [Master ESP32]
                    |
         +---------+---------+
         |                   |
    RS485 Bus A         RS485 Bus B
    [Slave 1-8]         [Slave 9-16]
```

This requires the standalone multi-bus master (in `Tools/RS485/master/master.ino`). CockpitOS's integrated master supports one bus, but you can run a dedicated ESP32 as a multi-bus master alongside CockpitOS.

### Topology 4: Mixed Transport Cockpit

```
PC <-- WiFi --> [Master ESP32 + IFEI Panel + Gauges]
                    |
              RS485 Bus
              [Slave 1: UFC buttons]
              [Slave 2: Landing gear + lights]
              [Slave 3: Warning panel LEDs]

PC <-- USB --> [Standalone ESP32-S3: Throttle axes + encoders]
```

The master handles its own TFT displays and gauges locally via WiFi, distributes switch/LED data to simple slave panels over RS485, and a separate USB-connected ESP32 handles analog axes that need the lowest possible latency.

**This is the sweet spot for a full cockpit build**: WiFi for the main panel (no USB cable), RS485 for the many small switch/LED panels (one cable run), USB for the few latency-critical analog controls.

### Topology 5: RS485 for Expansion, WiFi for Debug

A unique CockpitOS advantage: when a slave uses RS485 as its transport, the WiFi radio is free for debug logging. Enable `USE_DCSBIOS_WIFI=0` (it's the RS485 that talks to DCS), but you can still use WiFi-based debug output (`debugPrint` routed to UDP).

This means you can monitor every slave's internal state, poll counts, export data rates, and checksum errors over WiFi while RS485 handles the real-time data. Arduino RS485 setups don't have this capability at all since the AVR's single serial port is used for RS485, leaving no debug channel.

To take advantage of this, configure debug output in `Config.h`:
```c
#define DEBUG_OUTPUT_MODE    1    // 1=WiFi UDP debug
```

Then use the CockpitOS UDP debug console tool to monitor all slaves simultaneously from your PC.

---

## Debugging and Diagnostics

### Status Logging

Both master and slave print periodic status to the debug channel:

**Master**:
```
[RS485M] Polls=1250 Resp=98.5% Bcasts=245 Cmds=12 Slaves=3 Raw=0
```

- **Polls**: Total polls sent
- **Resp**: Response rate (should be 95-100% for healthy slaves)
- **Bcasts**: Broadcast packets sent
- **Cmds**: Input commands received from slaves
- **Slaves**: Number of online slaves
- **Raw/Queue**: Pending broadcast data (relay buffer or change queue)

**Slave**:
```
[RS485S] Polls=450 Bcasts=90 Export=23040 Cmds=5 TxPend=0
[RS485S] Last poll: 8 ms ago
```

- **Polls**: How many times we've been polled
- **Bcasts**: Broadcast packets received
- **Export**: Total export bytes received
- **Cmds**: Commands sent back to master
- **TxPend**: Bytes waiting to send (should be 0 most of the time)
- **Last poll**: Time since last poll (should be <50ms for healthy bus)

### Print Full Status

Call `RS485Master_printStatus()` or `RS485Slave_printStatus()` for a comprehensive dump including online slave list, mode, buffer states, and all counters.

### Common Diagnostic Patterns

| Symptom | Likely Cause |
|---------|-------------|
| `Slaves=0` on master | Wiring issue, baud mismatch, or slaves not powered |
| `Resp=0%` on master | DE pin misconfigured, TX/RX swapped, or no common ground |
| `Last poll: 4294967295 ms ago` on slave | Slave never received a poll (wiring or address mismatch) |
| `Bcasts=0` on slave | Master not broadcasting (no DCS connection, or smart mode filtering) |
| Slave `Export` count stuck at 0 | Master connected but DCS mission not running |
| Slave `Cmds` stuck at 0 | No inputs configured, or InputMapping.h issue |

### Verbose Debug Mode

For deep debugging, enable verbose logging in the respective config header:

```c
// RS485Config.h (master) or RS485SlaveConfig.h (slave)
#define RS485_DEBUG_VERBOSE    1    // Logs every packet (very chatty!)
```

This logs every poll, response, broadcast, and timeout. Only use for debugging specific issues.

---

## Performance and Limits

### Bus Capacity

At 250000 baud (8N1 = 10 bits/byte):

| Metric | Value |
|--------|-------|
| Raw throughput | 25,000 bytes/second |
| One byte on wire | 40 microseconds |
| Poll cycle (empty response) | ~1.2ms |
| Poll cycle (20-byte response) | ~1.2ms |
| Broadcast (64 bytes) | ~2.8ms |
| Full cycle (broadcast + poll) | ~4ms |

### Slave Polling Rate

| Slaves | Polls/Slave/Second | Adequate For |
|--------|-------------------|--------------|
| 1 | ~250 | Everything |
| 4 | ~60 | Switches, encoders, LEDs |
| 8 | ~30 | Switches, LEDs |
| 16 | ~15 | Switches, LEDs (marginal for fast encoders) |
| 32 | ~8 | Simple switch panels only |

For cockpit builds, 4-8 slaves is the sweet spot. Beyond 16, consider multiple buses or mixing RS485 with WiFi.

### Latency Budget

| Path | Typical Latency |
|------|----------------|
| Switch press to slave TX buffer | <1ms (ISR-driven input polling) |
| Slave TX buffer to master poll | 4-30ms (depends on slave count) |
| Master poll to DCS World | 1-10ms (depends on transport) |
| **Total switch-to-DCS** | **5-40ms** |
| DCS export to master | 1-33ms (30Hz export rate) |
| Master broadcast to slave | <3ms |
| Slave export parse to LED update | <1ms |
| **Total DCS-to-LED** | **2-37ms** |

All latencies are well within human perception thresholds (~100ms for visual, ~50ms for tactile).

---

## Troubleshooting

### Nothing works (no slave responses)

1. **Check wiring**: A goes to A, B goes to B on every device. Common ground connected.
2. **Check baud rate**: Must be 250000 on all devices.
3. **Check pins**: TX/RX are from the ESP32's perspective. TX goes to the transceiver's DI (Data In), RX goes to RO (Receiver Out).
4. **Check DE pin**: If using manual DE, make sure it's connected to both DE and RE (active-low) on the MAX485, tied together.
5. **Check addresses**: Each slave must have a unique address. Master polls addresses 1 through `RS485_MAX_SLAVE_ADDRESS`.
6. **Check UART**: `RS485_UART_NUM` must not conflict with USB/debug (avoid UART0).

### Slave receives broadcasts but doesn't respond to polls

- DE pin not toggling (check with oscilloscope or LED on DE pin)
- Slave address doesn't match what master is polling
- ISR mode may have initialization issue on specific chip: try `RS485_USE_ISR_MODE=0` as fallback

### Intermittent data corruption

- Missing termination resistor on long runs (>5m)
- No common ground between devices
- Power supply noise coupling into RS485 lines
- If using auto-direction transceivers: try adding `RS485_TX_COOLDOWN_DELAY_US=50`

### Master shows 0% response rate but slave says it's receiving polls

- TX/RX wires may be swapped on one end
- DE pin stuck high on slave (slave is permanently transmitting, jamming bus)
- Slave's response arrives after master's 1ms timeout: increase `RS485_POLL_TIMEOUT_US`

### Smart Mode: slaves not updating

- The master's label set must include all addresses the slaves need
- Regenerate with `python generate_data.py` after changing `selected_panels.txt`
- Try Relay Mode first to confirm basic communication works

---

*CockpitOS RS485 Guide - February 2026*
