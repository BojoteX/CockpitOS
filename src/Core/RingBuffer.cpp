// RingBuffer.cpp - Implements a simple ring buffer for transport

#include "../Globals.h"

#if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
  #include "../WiFiDebug.h"
#endif

// Ring buffer is mutually exclusive for DCSBIOS WiFi and Incoming HID Data. Only ONE can be active
#if (USE_DCSBIOS_WIFI && USE_DCSBIOS_USB)
#error "Invalid configuration: USE_DCSBIOS_WIFI and USE_DCSBIOS_USB cannot both be set to 1. Only one can be enabled at a time because they share the receive ring buffer."
#endif

// ═══════════════════════════════════════════════════════════════════════════
// REMOTE BOOTLOADER TRIGGER
// Called when magic packet "COCKPITOS:REBOOT:<target>\n" matches this device
// ═══════════════════════════════════════════════════════════════════════════
#include "esp_system.h"

// S2/S3 with TinyUSB have a ready-made function
#if (CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3) && (ARDUINO_USB_MODE == 0)
#include "esp32-hal-tinyusb.h"
#define USE_USB_PERSIST_RESTART 1
#else
#define USE_USB_PERSIST_RESTART 0
#endif

// Chip-specific register definitions
#if CONFIG_IDF_TARGET_ESP32S2
#include "soc/rtc_cntl_reg.h"
#define FORCE_DOWNLOAD_REG  RTC_CNTL_OPTION1_REG
#define FORCE_DOWNLOAD_BIT  RTC_CNTL_FORCE_DOWNLOAD_BOOT
#define BOOTLOADER_SUPPORTED 1
#elif CONFIG_IDF_TARGET_ESP32S3
#include "soc/rtc_cntl_reg.h"
#define FORCE_DOWNLOAD_REG  RTC_CNTL_OPTION1_REG
#define FORCE_DOWNLOAD_BIT  RTC_CNTL_FORCE_DOWNLOAD_BOOT
#define BOOTLOADER_SUPPORTED 1
#elif CONFIG_IDF_TARGET_ESP32C3
#include "soc/rtc_cntl_reg.h"
#define FORCE_DOWNLOAD_REG  RTC_CNTL_OPTION1_REG
#define FORCE_DOWNLOAD_BIT  RTC_CNTL_FORCE_DOWNLOAD_BOOT
#define BOOTLOADER_SUPPORTED 1
#elif CONFIG_IDF_TARGET_ESP32C6
#include "soc/lp_aon_reg.h"
#define FORCE_DOWNLOAD_REG  LP_AON_SYS_CFG_REG
#define FORCE_DOWNLOAD_BIT  LP_AON_FORCE_DOWNLOAD_BOOT
#define BOOTLOADER_SUPPORTED 1
#elif CONFIG_IDF_TARGET_ESP32H2
#include "soc/lp_aon_reg.h"
#define FORCE_DOWNLOAD_REG  LP_AON_SYS_CFG_REG
#define FORCE_DOWNLOAD_BIT  LP_AON_FORCE_DOWNLOAD_BOOT
#define BOOTLOADER_SUPPORTED 1
#else
    // ESP32 Classic - hardware limitation, cannot enter bootloader programmatically
#define BOOTLOADER_SUPPORTED 0
#endif

#if BOOTLOADER_SUPPORTED && !USE_USB_PERSIST_RESTART
// Shutdown handler - executes at the RIGHT time during restart sequence
static void IRAM_ATTR bootloader_shutdown_handler(void) {
    REG_WRITE(FORCE_DOWNLOAD_REG, FORCE_DOWNLOAD_BIT);
}
#endif

void enterBootloaderMode() {

    debugPrintln("🔄 [BOOTLOADER] Entering firmware download mode...");
    delay(100);  // Let debug output flush

#if !BOOTLOADER_SUPPORTED
    // ESP32 Classic cannot do this
    debugPrintln("❌ [BOOTLOADER] ESP32 Classic cannot enter bootloader programmatically");
    debugPrintln("   Hardware limitation - use physical BOOT button or OTA updates");
    return;
#else

#if USE_USB_PERSIST_RESTART
    // S2/S3 with TinyUSB: Use the built-in API (handles USB peripheral properly)
    usb_persist_restart(RESTART_BOOTLOADER);
#else
    // All other supported chips: Use shutdown handler approach
    esp_err_t err = esp_register_shutdown_handler(bootloader_shutdown_handler);
    if (err == ESP_OK) {
        esp_restart();
    }
    else {
        debugPrintf("❌ [BOOTLOADER] Failed to register shutdown handler: %d\n", err);
    }
#endif

#endif

    // Should never reach here
    while (1) { delay(100); }
}
// ═══════════════════════════════════════════════════════════════════════════


#if (USE_DCSBIOS_WIFI || USE_DCSBIOS_USB || USE_DCSBIOS_BLUETOOTH)


// ═══════════════════════════════════════════════════════════════════════════
// MAGIC PACKET DETECTION
// Called from both WiFi and USB ring buffer entry points
// ═══════════════════════════════════════════════════════════════════════════
static void checkBootloaderMagicPacket(const uint8_t* data, size_t len) {

    static constexpr char MAGIC_PREFIX[] = "COCKPITOS:REBOOT:";
    static constexpr size_t PREFIX_LEN = 17;

    // Accept both raw (WiFi) and padded (USB HID) packets
    if (len >= 19 && len <= 64) {
        if (memcmp(data, MAGIC_PREFIX, PREFIX_LEN) == 0) {

            // Find newline in remaining bytes (don't assume position)
            const uint8_t* targetStart = data + PREFIX_LEN;
            size_t maxTargetLen = len - PREFIX_LEN;
            size_t targetLen = 0;

            // Search for newline terminator
            for (size_t i = 0; i < maxTargetLen && i < 32; i++) {
                if (targetStart[i] == '\n') {
                    targetLen = i;
                    break;
                }
            }

            // Must have found a newline and target must be at least 1 char
            if (targetLen >= 1) {
                const char* target = (const char*)targetStart;
                bool shouldReboot = false;

                if (targetLen == 1 && target[0] == '*') {
                    shouldReboot = true;
                }
                else if (targetLen == strlen(LABEL_SET_STR) &&
                    memcmp(target, LABEL_SET_STR, targetLen) == 0) {
                    shouldReboot = true;
                }
                else if (targetLen == 6 && target[0] == '0' && target[1] == 'x') {
                    char pidStr[7];
                    snprintf(pidStr, sizeof(pidStr), "0x%04X", USB_PID);
                    if (memcmp(target, pidStr, 6) == 0) {
                        shouldReboot = true;
                    }
                }

                if (shouldReboot) {
                    enterBootloaderMode();
                }
            }
        }
    }
}





// My Incoming DCS stream UDP RingBuffer

// --- DCS Stream UDP In Ring Buffer (Host → Device) ---

// Buffer and indices
static DcsUdpRingMsg dcsUdpRing[DCS_UDP_RINGBUF_SIZE];
static volatile uint8_t dcsUdpHead = 0, dcsUdpTail = 0;

// Stats
static volatile uint32_t dcsUdpRecvTotalBytes = 0;
static volatile uint32_t dcsUdpRecvMsgCount = 0;
static volatile size_t   dcsUdpRecvMsgMaxLen = 0;
static volatile uint32_t dcsUdpRecvOverflow = 0;
static volatile size_t   dcsUdpRecvHighWater = 0;

inline static bool dcsUdpRingFull()  { return ((dcsUdpHead + 1) % DCS_UDP_RINGBUF_SIZE) == dcsUdpTail; }
inline static bool dcsUdpRingEmpty() { return dcsUdpHead == dcsUdpTail; }

uint32_t dcsUdpRecvGetOverflow(void)      { return dcsUdpRecvOverflow; }
size_t   dcsUdpRecvGetHighWater(void)     { return dcsUdpRecvHighWater; }
size_t   dcsUdpRecvGetPending(void)       { return dcsUdpRingbufPending(); }
float    dcsUdpRecvAvgMsgLen(void)        { return (dcsUdpRecvMsgCount > 0) ? ((float)dcsUdpRecvTotalBytes / dcsUdpRecvMsgCount) : 0.0f; }
size_t   dcsUdpRecvMaxMsgLen(void)        { return dcsUdpRecvMsgMaxLen; }

size_t dcsUdpRingbufPending() {
    if (dcsUdpHead >= dcsUdpTail) return dcsUdpHead - dcsUdpTail;
    return DCS_UDP_RINGBUF_SIZE - (dcsUdpTail - dcsUdpHead);
}

size_t dcsUdpRingbufAvailable() {
    if (dcsUdpHead >= dcsUdpTail)
        return DCS_UDP_RINGBUF_SIZE - (dcsUdpHead - dcsUdpTail) - 1;
    else
        return (dcsUdpTail - dcsUdpHead) - 1;
}

bool dcsUdpRingbufPop(DcsUdpRingMsg* out) {
    if (dcsUdpRingEmpty()) return false;
    *out = dcsUdpRing[dcsUdpTail];
    dcsUdpTail = (dcsUdpTail + 1) % DCS_UDP_RINGBUF_SIZE;
    return true;
}

void dcsUdpRingbufPush(const uint8_t* data, size_t len, bool isLastChunk) {
    if (dcsUdpRingFull()) {
        dcsUdpRecvOverflow++;
        debugPrintln("❌ [RING BUFFER] Ring buffer is FULL, increase DCS_UDP_RINGBUF_SIZE");
        return;
    }
    if (len > DCS_UDP_PACKET_MAXLEN) len = DCS_UDP_PACKET_MAXLEN;
    memcpy(dcsUdpRing[dcsUdpHead].data, data, len);
    dcsUdpRing[dcsUdpHead].len = len;
    dcsUdpRing[dcsUdpHead].isLastChunk = isLastChunk;
    dcsUdpHead = (dcsUdpHead + 1) % DCS_UDP_RINGBUF_SIZE;

    size_t pending = dcsUdpRingbufPending();
    if (pending > dcsUdpRecvHighWater) dcsUdpRecvHighWater = pending;

    dcsUdpRecvTotalBytes += len;
    dcsUdpRecvMsgCount++;
    if (len > dcsUdpRecvMsgMaxLen) dcsUdpRecvMsgMaxLen = len;    
}

void dcsUdpRingbufPushChunked(const uint8_t* data, size_t len) {

	// Check for bootloader magic packet
    checkBootloaderMagicPacket(data, len);

    const size_t max_data = DCS_UDP_PACKET_MAXLEN;
    size_t needed = (len + max_data - 1) / max_data;
    if (dcsUdpRingbufAvailable() < needed) {
        dcsUdpRecvOverflow++;
        debugPrintln("❌ [RING BUFFER] Available space was less than required, increase DCS_UDP_RINGBUF_SIZE");
        return;
    }
    size_t pos = 0;
    for (size_t chunk_num = 0; chunk_num < needed; ++chunk_num) {
        size_t chunk_len = (len - pos > max_data) ? max_data : (len - pos);
        bool last = (chunk_num == needed - 1);
        dcsUdpRingbufPush(data + pos, chunk_len, last);
        pos += chunk_len;
    }
}

// My Outgoing USB RingBuffer

// --- RAW USB Out Ring Buffer (Device → Host) ---
static DcsRawUsbOutRingMsg dcsRawUsbOutRing[DCS_USB_RINGBUF_SIZE];
static volatile uint8_t dcsRawUsbOutHead = 0, dcsRawUsbOutTail = 0;

// Stats
static volatile uint32_t dcsRawUsbOutOverflow = 0;
static volatile uint32_t dcsRawUsbOutMsgCount = 0;
static volatile uint32_t dcsRawUsbOutTotalBytes = 0;
static volatile size_t   dcsRawUsbOutMsgMaxLen = 0;
static volatile size_t   dcsRawUsbOutHighWater = 0;

inline static bool dcsRawUsbOutRingFull()  { return ((dcsRawUsbOutHead + 1) % DCS_USB_RINGBUF_SIZE) == dcsRawUsbOutTail; }
inline static bool dcsRawUsbOutRingEmpty() { return dcsRawUsbOutHead == dcsRawUsbOutTail; }

uint32_t dcsRawUsbOutGetOverflow(void)      { return dcsRawUsbOutOverflow; }
size_t   dcsRawUsbOutGetHighWater(void)     { return dcsRawUsbOutHighWater; }
size_t   dcsRawUsbOutGetPending(void)       { return dcsRawUsbOutRingbufPending(); }
float    dcsRawUsbOutAvgMsgLen(void)        { return (dcsRawUsbOutMsgCount > 0) ? ((float)dcsRawUsbOutTotalBytes / dcsRawUsbOutMsgCount) : 0.0f; }
size_t   dcsRawUsbOutMaxMsgLen(void)        { return dcsRawUsbOutMsgMaxLen; }

size_t dcsRawUsbOutRingbufPending() {
    if (dcsRawUsbOutHead >= dcsRawUsbOutTail) return dcsRawUsbOutHead - dcsRawUsbOutTail;
    return DCS_USB_RINGBUF_SIZE - (dcsRawUsbOutTail - dcsRawUsbOutHead);
}

size_t dcsRawUsbOutRingbufAvailable() {
    if (dcsRawUsbOutHead >= dcsRawUsbOutTail)
        return DCS_USB_RINGBUF_SIZE - (dcsRawUsbOutHead - dcsRawUsbOutTail) - 1;
    else
        return (dcsRawUsbOutTail - dcsRawUsbOutHead) - 1;
}

bool dcsRawUsbOutRingbufPop(DcsRawUsbOutRingMsg* out) {
    if (dcsRawUsbOutRingEmpty()) return false;
    *out = dcsRawUsbOutRing[dcsRawUsbOutTail];
    dcsRawUsbOutTail = (dcsRawUsbOutTail + 1) % DCS_USB_RINGBUF_SIZE;
    return true;
}

void dcsRawUsbOutRingbufPush(const uint8_t* data, size_t len, bool isLastChunk) {
    if (dcsRawUsbOutRingFull()) {
        dcsRawUsbOutOverflow++;
        debugPrintln("❌ [RING BUFFER] Outgoing message overflow! increase DCS_USB_RINGBUF_SIZE");
        return;
    }
    if (len > DCS_USB_PACKET_MAXLEN) len = DCS_USB_PACKET_MAXLEN;
    memcpy(dcsRawUsbOutRing[dcsRawUsbOutHead].data, data, len);
    dcsRawUsbOutRing[dcsRawUsbOutHead].len = len;
    dcsRawUsbOutRing[dcsRawUsbOutHead].isLastChunk = isLastChunk;
    dcsRawUsbOutHead = (dcsRawUsbOutHead + 1) % DCS_USB_RINGBUF_SIZE;

    size_t pending = dcsRawUsbOutRingbufPending();
    if (pending > dcsRawUsbOutHighWater) dcsRawUsbOutHighWater = pending;

    dcsRawUsbOutTotalBytes += len;
    dcsRawUsbOutMsgCount++;
    if (len > dcsRawUsbOutMsgMaxLen) dcsRawUsbOutMsgMaxLen = len;
}

void dcsRawUsbOutRingbufPushChunked(const uint8_t* data, size_t len) {

    // Check for bootloader magic packet
    checkBootloaderMagicPacket(data, len);

    const size_t max_data = DCS_USB_PACKET_MAXLEN;
    size_t needed = (len + max_data - 1) / max_data;
    if (dcsRawUsbOutRingbufAvailable() < needed) {
        dcsRawUsbOutOverflow++;
        debugPrintln("❌ [RING BUFFER] Outgoing message queue would overflow, skipping. Increase DCS_USB_RINGBUF_SIZE");
        return;
    }
    size_t pos = 0;
    for (size_t chunk_num = 0; chunk_num < needed; ++chunk_num) {
        size_t chunk_len = (len - pos > max_data) ? max_data : (len - pos);
        bool last = (chunk_num == needed - 1);
        dcsRawUsbOutRingbufPush(data + pos, chunk_len, last);
        pos += chunk_len;
    }
}

void dumpUsbOutRingBuffer() {
#if USE_DCSBIOS_USB
    char outbuf[1024];
    size_t olen = 0;
    size_t i = dcsRawUsbOutTail;
    size_t count = dcsRawUsbOutRingbufPending();

    olen += snprintf(outbuf + olen, sizeof(outbuf) - olen, "RING BUFFER (pending %u):\n", (unsigned)count);

    for (size_t n = 0; n < count; ++n) {
        DcsRawUsbOutRingMsg* msg = &dcsRawUsbOutRing[i];

        // Print as plain string (up to first non-printable or null, or up to len)
        char str[80];
        size_t s = 0;
        for (; s < msg->len && s < sizeof(str) - 1; ++s) {
            char c = msg->data[s];
            if (c < 32 || c > 126) break;
            str[s] = c;
        }
        str[s] = 0;

        olen += snprintf(outbuf + olen, sizeof(outbuf) - olen, "[%u] \"%s\"   len=%u  last=%d\n",
            (unsigned)i, str, (unsigned)msg->len, msg->isLastChunk ? 1 : 0);

        i = (i + 1) % DCS_USB_RINGBUF_SIZE;
        if (olen > sizeof(outbuf) - 80) break; // avoid overflow
    }
    #if USE_DCSBIOS_WIFI || DEBUG_USE_WIFI
        wifiDebugSendRaw(outbuf, olen);
    #else
        #if (USE_DCSBIOS_SERIAL || VERBOSE_MODE_SERIAL_ONLY || VERBOSE_MODE)
            writeToConsole(outbuf, olen);
        #endif
    #endif
#endif
}

#endif // (USE_DCSBIOS_WIFI || USE_DCSBIOS_USB || USE_DCSBIOS_BLUETOOTH)