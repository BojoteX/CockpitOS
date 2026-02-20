# How To: Wire RS485 Network

RS485 networking connects multiple ESP32 panels on a shared bus so they all receive DCS-BIOS data from a single master. This eliminates the need for separate USB/WiFi connections per panel and works reliably over long cable runs.

---

## What RS485 Is

RS485 is a differential signaling standard that uses two wires (A and B) to transmit data. Because it measures the voltage difference between the two wires rather than the absolute voltage, it is highly resistant to electrical noise -- ideal for cockpit builds with servo motors, solenoids, and other noise sources.

**Key characteristics:**
- Up to 32 devices on one bus (with standard transceivers)
- Cable runs up to 1200 meters (4000 feet) at lower baud rates
- Noise-immune differential signaling
- Half-duplex (one device transmits at a time)

---

## When to Use RS485

Use RS485 when:
- You have **multiple ESP32 panels** and want to share one DCS-BIOS connection
- You have **long cable runs** between panels (more than a few meters)
- You want to **reduce cost** by not needing a USB cable or WiFi connection per panel
- You want **interoperability** with Arduino DCS-BIOS RS485 devices on the same bus

---

## What You Need

| Item | Qty | Notes |
|---|---|---|
| RS485 transceiver module | One per ESP32 | MAX485, SP3485, or auto-direction modules. Auto-direction modules (no DE/RE pin) are easiest. |
| Twisted pair cable | As needed | CAT5/CAT6 ethernet cable works well. Use one twisted pair for A/B. |
| 120 ohm termination resistors | 2 | One at each end of the bus. Many RS485 modules have a built-in termination jumper. |
| ESP32 boards | 1 master + N slaves | Any ESP32 variant works. The master needs one of the standard transports (USB/WiFi/Serial) too. |

---

## Step 1: Wire the Bus

### Basic Two-Wire Connection

Every device on the bus connects A-to-A and B-to-B. The bus is a single twisted pair daisy-chained from device to device.

```
+----------------------------------------------------------------------+
|  RS485 BUS TOPOLOGY                                                   |
+----------------------------------------------------------------------+
|                                                                      |
|  [120R]                                              [120R]          |
|    |                                                    |            |
|  +-+--------+     +----------+     +----------+     +--+-------+    |
|  |  MASTER  |     | SLAVE 1  |     | SLAVE 2  |     | SLAVE 3  |    |
|  |  ESP32   |     |  ESP32   |     |  ESP32   |     |  ESP32   |    |
|  |          |     |          |     |          |     |          |    |
|  |  RS485   |     |  RS485   |     |  RS485   |     |  RS485   |    |
|  | Module   |     | Module   |     | Module   |     | Module   |    |
|  +--+---+---+     +--+---+---+     +--+---+---+     +--+---+---+    |
|     |   |            |   |            |   |            |   |        |
|     A   B --- A   B --   A   B --- A   B --   A   B -- A   B       |
|                                                                      |
|     <=============== TWISTED PAIR CABLE ===============>              |
|                                                                      |
+----------------------------------------------------------------------+
```

### Wiring Each ESP32 to Its RS485 Module

```
                    ESP32                       RS485 Module
                 +---------+                  +----------+
                 |         |                  |          |
                 | TX pin  +------------------| DI (TX)  |
                 | RX pin  +------------------| RO (RX)  |
                 |         |                  |          |
                 | DE pin  +------------------| DE/RE    |  (manual direction only;
                 |         |                  |          |   set to -1 for auto modules)
                 |         |                  |          |
                 |  3.3V   +------------------| VCC      |
                 |   GND   +------------------| GND      |
                 |         |                  |     A  B +---> To bus
                 +---------+                  +----------+
```

### Important Wiring Rules

1. **Do not swap A and B.** If one device has them swapped, nothing on the bus will work.
2. **Use twisted pair cable.** A and B must be a twisted pair for noise rejection.
3. **Add termination resistors** (120 ohm) at each physical end of the bus. Do not add them in the middle.
4. **Share a common ground** between all devices if possible (using a third wire).

---

## Step 2: Configure the Master

The master is a normal CockpitOS panel that connects to DCS-BIOS via USB, WiFi, or Serial. It additionally forwards data to all slaves over RS485.

### Compiler Tool Configuration (Master)

In the Compiler Tool, select **Role / Transport** and choose **RS485 Master**. The tool walks you through a wizard: first select the PC-side transport (USB, WiFi, or Serial), then configure RS485 master settings. All Config.h flags are written automatically:

```cpp
// These are set by the Compiler Tool -- shown for reference only
#define USE_DCSBIOS_USB             1   // (your chosen PC transport)
#define USE_DCSBIOS_WIFI            0
#define USE_DCSBIOS_SERIAL          0

#define RS485_MASTER_ENABLED        1
#define RS485_SLAVE_ENABLED         0
```

### CustomPins.h Settings (Master)

```cpp
// RS485 transceiver pins
#define RS485_TX_PIN                17    // ESP32 TX -> RS485 DI
#define RS485_RX_PIN                18    // ESP32 RX <- RS485 RO
#define RS485_DE_PIN                -1    // Set to GPIO if using manual DE/RE control
                                          // Set to -1 for auto-direction modules
```

### Optional Master Settings

The Compiler Tool's RS485 master wizard configures Smart Mode and Max Slave Address. The remaining settings below are advanced and can be changed manually in `Config.h` if needed:

| Setting | Default | Description | Configured By |
|---|---|---|---|
| `RS485_SMART_MODE` | `0` | When `1`, master filters data and only forwards addresses that slaves need. Reduces bandwidth. | Compiler Tool |
| `RS485_MAX_SLAVE_ADDRESS` | `127` | Highest slave address to poll. Lower this to reduce polling overhead. | Compiler Tool |
| `RS485_USE_TASK` | `1` | `1` = dedicated FreeRTOS task (best for USB/Serial). `0` = run in main loop (best for WiFi). | Manual (Config.h) |
| `RS485_TASK_CORE` | `0` | CPU core for the RS485 task on dual-core boards. | Manual (Config.h) |

---

## Step 3: Configure Each Slave

Slaves receive DCS-BIOS data from the master over RS485. They do not need USB, WiFi, or Serial to the PC.

### Compiler Tool Configuration (Slave)

In the Compiler Tool, select **Role / Transport** and choose **RS485 Slave**. The tool prompts you for the slave address (1-126) and writes all Config.h flags automatically:

```cpp
// These are set by the Compiler Tool -- shown for reference only
#define USE_DCSBIOS_USB             0
#define USE_DCSBIOS_WIFI            0
#define USE_DCSBIOS_SERIAL          0

#define RS485_MASTER_ENABLED        0
#define RS485_SLAVE_ENABLED         1
#define RS485_SLAVE_ADDRESS         1    // Unique per slave (1-126)
```

**Every slave must have a unique address** (1-126). If two slaves share the same address, bus collisions will occur and neither will work reliably.

### CustomPins.h Settings (Slave)

Same pin definitions as the master:

```cpp
#define RS485_TX_PIN                17
#define RS485_RX_PIN                18
#define RS485_DE_PIN                -1
```

### Each Slave Has Its Own Label Set

Each slave has its own label set folder in `src/LABELS/` with its own InputMapping.h, LEDMapping.h, and CustomPins.h. The label set defines what hardware is connected to that specific slave panel.

---

## Step 4: Understand the Data Flow

```
+----------------------------------------------------------------------+
|  RS485 DATA FLOW                                                      |
+----------------------------------------------------------------------+
|                                                                      |
|  DCS World (PC)                                                      |
|       |                                                              |
|       v                                                              |
|  DCS-BIOS stream (USB/WiFi/Serial)                                   |
|       |                                                              |
|       v                                                              |
|  +----------+                                                        |
|  |  MASTER  |  Receives DCS-BIOS data + runs its own panel           |
|  |  ESP32   |                                                        |
|  +----+-----+                                                        |
|       |                                                              |
|       v  (RS485 broadcast)                                           |
|  +----+-----+-----+-----+                                           |
|  |    |     |     |     |                                            |
|  v    v     v     v     v                                            |
| S1   S2    S3   S4   S5      Slaves receive data, run their panels   |
|  |    |     |     |     |                                            |
|  +----+-----+-----+-----+                                           |
|       |                                                              |
|       v  (RS485 poll responses)                                      |
|  +----+-----+                                                        |
|  |  MASTER  |  Collects input commands from slaves                   |
|  +----+-----+  and forwards them to DCS-BIOS                        |
|       |                                                              |
|       v                                                              |
|  DCS World (PC)                                                      |
|                                                                      |
+----------------------------------------------------------------------+
```

### Smart Mode vs. Relay Mode

| Mode | Config | Behavior |
|---|---|---|
| **Relay Mode** (default) | `RS485_SMART_MODE 0` | Master relays the entire DCS-BIOS stream to all slaves. Simpler, more bandwidth used. |
| **Smart Mode** | `RS485_SMART_MODE 1` | Master filters the stream and only forwards data that at least one slave has subscribed to. Lower bandwidth, slightly higher latency on the master. |

For most builds with a small number of slaves, Relay Mode is fine. Smart Mode becomes beneficial when you have many slaves and want to reduce bus traffic.

---

## Step 5: Test the Network

1. **Flash the master** with its label set (RS485_MASTER_ENABLED=1)
2. **Flash each slave** with its own label set (RS485_SLAVE_ENABLED=1, unique address)
3. **Connect all devices** to the RS485 bus
4. **Power everything on**
5. **Start DCS** and load a mission

### Debugging

Enable RS485 debug output on the master:

```cpp
// In Config.h (master only, for testing)
#define RS485_DEBUG_VERBOSE         1    // Logs every poll/response (very verbose)
#define RS485_STATUS_INTERVAL_MS    10000 // Status banner every 10 seconds
```

Use the WiFi debug console (`CONSOLE_UDP_debug.py`) on the master to monitor bus activity.

---

## Step 6: Interoperability

CockpitOS RS485 is interoperable with Arduino DCS-BIOS RS485 devices on the same bus. You can mix CockpitOS ESP32 slaves with Arduino Mega/Nano slaves, as long as they all share the same baud rate and protocol.

---

## Performance Reference

| Parameter | Value |
|---|---|
| Default baud rate | 250000 bps |
| Maximum slaves (standard transceiver) | 31 |
| Typical polling latency per slave | 2-5ms |
| Total bus latency (10 slaves) | 20-50ms |
| Maximum cable length at 250Kbps | ~300m |

---

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| No communication at all | A and B wires swapped | Swap A and B on one end; verify all devices have the same A/B orientation |
| Intermittent communication | Missing termination resistors, long unterminated stubs | Add 120 ohm termination at both ends of the bus; keep stubs short |
| Slave not responding | Wrong slave address, address conflict | Verify each slave has a unique address; check RS485_SLAVE_ADDRESS in Config.h |
| Master receives garbage | Baud rate mismatch, noise on bus | Ensure all devices use the same baud rate; use twisted pair cable |
| Slave receives data but inputs do not send | Polling not reaching slave | Check RS485_MAX_SLAVE_ADDRESS is >= your highest slave address |
| Data corruption on long runs | No termination, single-ended wiring | Add termination resistors; use twisted pair; add a common ground wire |

---

## Timing Configuration Reference

| Setting | Default | Description |
|---|---|---|
| `RS485_TX_PRE_DE_DELAY_US` | `40` | Delay before slave responds. Prevents collisions with slow masters. |
| `RS485_TX_WARMUP_DELAY_US` | `0` | Delay after DE assert before TX (manual direction control). |
| `RS485_TX_WARMUP_AUTO_DELAY_US` | `0` | Delay before TX for auto-direction modules. |
| `RS485_STATUS_INTERVAL_MS` | `60000` | Interval between status banners in debug output. |

---

*See also: [Config Reference](../Reference/Config.md) | [Transport Modes](../Reference/Transport-Modes.md) | [Advanced RS485 Deep Dive](../Advanced/RS485-Deep-Dive.md)*
