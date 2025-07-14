// Config.h - Configuration file for CockpitOS

#pragma once

// Here you need to #define ONE and ONLY one of possible LABEL sets to use with your panel, 
// ** ATTENTION ** REMEBER TO RUN generate_data.py (see LABELS directory) BEFORE compiling for each label set below 

// #define LABEL_SET_F16_TEST
// #define LABEL_SET_BATTERY_GAUGE
// #define LABEL_SET_IFEI_NO_VIDEO  
#define LABEL_SET_ALR67                            
// #define LABEL_SET_MAIN                                                      
// #define LABEL_SET_ALTIMETER                           
// #define LABEL_SET_ALL                           

// Here is where you tell the firmware which feature to use to SEND and RECEIVE data to DCS. Pure USB or WIFI. This methods COMPLETELY bypass Serial. If both are set to 0, then standard CDC / Serial is used. Both CAN NOT be set to 1 at the same time
#define USE_DCSBIOS_WIFI                          0 // Completely bypasses socat and uses WiFi to connect to DCS. 
// or (not both)
#define USE_DCSBIOS_USB                           1 // Completely bypasses socat and uses USB to connect to DCS. You need to run the CockpitOS Companion app on the host PC for this to work.

// Wi-Fi network credentials (used for WiFi remote Debug Console and DCSBIOS WiFi mode if selected)
#define WIFI_SSID                                 "MyHotspotNetwork" // Use a hotspot for local testing and debugging, but for production use your regular WiFi if you plan to enable USE_DCSBIOS_WIFI
#define WIFI_PASS                                 "TestingOnly"

// For production, ALL THESE should be set to 0. Use for debugging only. Most commonly used together are DEBUG_USE_WIFI + VERBOSE_MODE_WIFI_ONLY + DEBUG_PERFORMANCE
#define TEST_LEDS                                 0  // Interactive menu (via serial console) to test LEDs individually
#define IS_REPLAY                                 0  // Simulate a loopback DCS stream to check your panel is working and debug via Serial
#define DEBUG_ENABLED                             0  // Use it ONLY when identifying issues or troubleshooting
#define DEBUG_ENABLED_FOR_PCA_ONLY                0  // Use it ONLY when mapping Port/bit/mask in PCA9xxx devices
#define DEBUG_USE_WIFI                            0  // Uses WiFi to output VERBOSE and DEBUG messages
#define VERBOSE_MODE                              0  // Logs INFO messages to both Serial and UDP (very useful) 
#define VERBOSE_MODE_SERIAL_ONLY                  0  // Verbose will only output to Serial. 
#define VERBOSE_MODE_WIFI_ONLY                    0  // Verbose will only output to WiFi so Serial port is clean. Requires DEBUG_USE_WIFI
#define VERBOSE_PERFORMANCE_ONLY                  0  // Requires DEBUG_PERFORMANCE as well, this will only output perf snapshots, make sure you pick WIFI or SERIAL above and DEBUG_ENABLED is 0
#define DEBUG_PERFORMANCE                         0  // Shows a performance snapshot every x seconds (interval can be configured below)
#define DEBUG_PERFORMANCE_SHOW_TASKS              0  // Includes the current task list with the snapshot. Not really needed.
#define PERFORMANCE_SNAPSHOT_INTERVAL_SECONDS     60 // Interval between snapshots (in seconds)

// Advanced config, these settings have been carefully tuned for performance and stability, 
#define DCSBIOS_USE_LITE_VERSION                   1 // Set to 1 to use a LITE (local) version of the DCSBIOS Library. 0 Uses the Original unmodified Library (you'll need to install it)
#define ALLOW_CONCURRENT_CDC_AND_HID               0 // Careful, this should ALWAYS be set to 0, concurrent CDC + HID is unstable
#define GAMEPAD_REPORT_SIZE                       64 // Must match the HID descriptor, you should NEVER have to change this.
#define SERVO_UPDATE_FREQ_MS                      20 // For Analog servo instruments. Update rate in ms (as per servo specs)
#define SCAN_WIFI_NETWORKS                         0 // For debugging and see what networks the device sees (this outputs to Serial interface)
#define USE_WIRE_FOR_I2C                           0 // If set to 1 uses the Arduino compatible I2C Wire Library (slow)
#define PCA_FAST_MODE                              1 // Set to 1 to enable 400MHz PCA Bus FAST MODE 
#define SERIAL_RX_BUFFER_SIZE                    512 // This is the INCOMING buffer for DCS Data (in bytes) when using CDC / Serial. increase if you see OVERFLOW msg             
#define SERIAL_TX_TIMEOUT                          5 // in ms (Used for CDC receive stream timeouts)
#define HID_SENDREPORT_TIMEOUT                     5 // in ms (Only used with ESP32 Arduino Core HID.SendReport implementation)
#define CDC_TIMEOUT_RX_TX                          5 // in ms (Only used with CDC/Serial health checks)
#define DCS_UPDATE_RATE_HZ                        30 // DCSBIOS Loop update rate (used for DCS Keepalives, if enabled)
#define HID_REPORT_RATE_HZ                       250 // HID report sending rate in Hz. (used for HID Keepalives, if enabled)
#define POLLING_RATE_HZ                          250 // Panel/buttons update rate in Hz (125, 250, 500 Hz) during main loop. Leave at 250
#define DISPLAY_REFRESH_RATE_HZ                   60 // General refresh rate used for displays.  
#define DCS_KEEP_ALIVE_ENABLED                     0 // Should we enable DCSBIOS Keepalives? should ONLY used for testing
#define HID_KEEP_ALIVE_ENABLED                     0 // Should we enable HID Keepalives? should ONLY used for testing
#define MAX_TRACKED_RECORDS                      512 // default safety cap, DO NOT change
#define MAX_GROUPS                               128 // default safety cap, DO NOT change
#define VALUE_THROTTLE_MS                         50 // How long (ms) to skip sending the same value again (debouncing)
#define ANY_VALUE_THROTTLE_MS                     33 // How long (ms) to skip sending different values (prevents spamming the USB endpoint, while debouncing at the same time)
#define SELECTOR_DWELL_MS                        250 // Wait time (in ms) for stable selector value. Used by our dwell-time fitering logic
#define DCS_GROUP_MIN_INTERVAL_US               (1000000UL / DCS_UPDATE_RATE_HZ) // min spacing/separation between selector positions
#define HID_REPORT_MIN_INTERVAL_US              (1000000UL / HID_REPORT_RATE_HZ) // min spacing/separation between reports
#define DCS_KEEP_ALIVE_MS                       (1000 / DCS_UPDATE_RATE_HZ) // send PING 0 (ASCII command) every x ms (when using keep-alives)
#define HID_KEEP_ALIVE_MS                       (1000 / HID_REPORT_RATE_HZ) // send HID command every x ms (when using keep-alives)

// Serial Debug Ring Buffer
#define SERIAL_DEBUG_USE_RINGBUFFER               0 // Should be use a ring buffer for Serial Debug messages? not really necessary
#if SERIAL_DEBUG_USE_RINGBUFFER                    
  #define SERIAL_RINGBUF_SIZE                    64 // How many slots in our buffer
  #define SERIAL_MSG_MAXLEN                      64 // Max size for each slot
#else
  #define SERIAL_RINGBUF_SIZE                     0 // How many slots in our buffer
  #define SERIAL_MSG_MAXLEN                    1024 // Max size for each slot
#endif

// WiFi Debug Ring Buffer 
#define WIFI_DEBUG_USE_RINGBUFFER                 1 // Should be use a ring buffer for WiFi Debug messages? helps when using WiFi DCS Mode. If WiFi is not used, this value is ignored anyway. Also, if using CDC + WiFi Debug, this is REQUIRED to avoid CDC stalls
#if WIFI_DEBUG_USE_RINGBUFFER
  #define WIFI_DBG_SEND_RINGBUF_SIZE             64 // How many slots in our buffer
  #define WIFI_DBG_MSG_MAXLEN                    64 // Max size for each slot
#else
  #define WIFI_DBG_SEND_RINGBUF_SIZE              0 // How many slots in our buffer
  #define WIFI_DBG_MSG_MAXLEN                  1472 // Max size for each slot
#endif

// DCS Commands USB Send Ring Buffer (outgoing packets) - *MANDATORY* this one is REQUIRED to be set to send via USB pipe for transport (due to 64 byte report size limitation)
#define DCS_USB_RINGBUF_SIZE                     16  // Number of packets buffered (tune as needed)
#define DCS_USB_PACKET_MAXLEN                    64  // Max USB packet size (safe for DCS-BIOS)

// DCS UDP/USB Receive Ring Buffer (incoming packets) - *MANDATORY* when using USB mode, optional in WiFi UDP mode.
#define MAX_UDP_FRAMES_PER_DRAIN                  1  // Max number to hold in buffer before parsing (increase for bursty processing) 1 is deterministic, best.
#if USE_DCSBIOS_USB
  #define DCS_USE_RINGBUFFER                      1  // Should ALWAYS be 1 when USE_DCSBIOS_USB. DO NOT CHANGE 
  #define DCS_UDP_RINGBUF_SIZE                   64  // Number of USB packets buffered (tune as needed)
  #define DCS_UDP_PACKET_MAXLEN                  64  // Should ALWAYS be 64 when USE_DCSBIOS_USB 
#else // Used for incoming DCS stream via WiFi UDP (if enabled) 
  #define DCS_USE_RINGBUFFER                      1  // OPTIONAL. Should WiFi UDP use a ring buffer for the incoming DCS Stream data?
  #if DCS_USE_RINGBUFFER
    #define DCS_UDP_RINGBUF_SIZE                 64  // Number of UDP packets buffered (tune as needed)
    #define DCS_UDP_PACKET_MAXLEN                64  // Max UDP packet size (safe for Incoming UDP from DCS-BIOS)
  #else
    #define DCS_UDP_RINGBUF_SIZE                  0  // Number of packets buffered (tune as needed)
    #define DCS_UDP_PACKET_MAXLEN              1472  // Max UDP packet size (safe for DCS-BIOS)
  #endif
#endif

// Misc buffer sizes (Do not modify) this has been tweaked for max performance/througput.
#define TASKLIST_LINE_GENERAL_TMP_BUFFER        128 // DO NOT EXCEED 128, used by tasklist individual line printing
#define DEBUGPRINTF_GENERAL_TMP_BUFFER          256 // Buffer size for DEBUG out Serial messages when using DebugPrintf
#define SERIAL_DEBUG_BUFFER_SIZE                256 // Buffer size for DEBUG out Serial messages when using serialDebugPrintf
#define WIFI_DEBUG_BUFFER_SIZE                  256 // Buffer size for DEBUG out WiFi messages when using wifiDebugPrintf
#define UDP_TMPBUF_SIZE                        1472 // UDP Out Temp buffer
#define PERF_TMPBUF_SIZE                       1024 // Temp Buffer size for Performance Append logic
#define SERIAL_DEBUG_FLUSH_BUFFER_SIZE         2048 // Big enough for your largest full message (tune as needed)
#define SERIAL_DEBUG_OUTPUT_CHUNK_SIZE           64 // Final Serial.write will use this value to chunk writes
#define DCS_UDP_MAX_REASSEMBLED                1472 // Or whatever max UDP/Frame size you want

// Required for Descriptor handling. For custom commercial implementations using your own name (you are free to do so), just by mindful of PID, VID, Manufacturer and the actual USB_PRODUCT string. No need for attribution, you can use this software as per the included LICENSE
#define USB_VID		                           0xCAFE
#define USB_PID		                           0xCAF3
#define USB_SERIAL                           LABEL_SET
#define USB_PRODUCT                          "CockpitOS Panel"
#define USB_MANUFACTURER                     "CockpitOS Firmware Project"
#define USB_LANG_ID                          0x0409  // English (US)

// Automatically enables what we'll be using, no need to edit.
#if (ARDUINO_USB_CDC_ON_BOOT == 1)
  #define CONFIG_TINYUSB_CDC_ENABLED                1
  #define CONFIG_TINYUSB_CDC_MAX_PORTS              2  
#else
  #define CONFIG_TINYUSB_CDC_ENABLED                0
  #define CONFIG_TINYUSB_CDC_MAX_PORTS              0
#endif

// Advanced TinyUSB Config Options.
#define CFG_TUSB_DEBUG                             0  // Enabled TinyUSB debugging 
#define CONFIG_TINYUSB_CDC_RX_BUFSIZE             64
#define CONFIG_TINYUSB_CDC_TX_BUFSIZE             64
#define CONFIG_TINYUSB_HID_BUFSIZE                64  
#define CONFIG_TINYUSB_MSC_ENABLED                0
#define CONFIG_TINYUSB_HID_ENABLED                1
#define CONFIG_TINYUSB_MIDI_ENABLED               0
#define CONFIG_TINYUSB_VIDEO_ENABLED              0
#define CONFIG_TINYUSB_CUSTOM_CLASS_ENABLED       0
#define CONFIG_TINYUSB_DFU_RT_ENABLED             0
#define CONFIG_TINYUSB_DFU_ENABLED                0
#define CONFIG_TINYUSB_VENDOR_ENABLED             0
#define CONFIG_TINYUSB_NCM_ENABLED                0

// Define the Built-in LED if compiling with a board that does not define it. Only if you get errors about LED_BUILTIN not defined.
// #ifndef LED_BUILTIN
// #define LED_BUILTIN 15 // Default LED pin
// #endif