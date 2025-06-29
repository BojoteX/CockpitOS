// debugPrint.cpp

#include "../Globals.h"

#if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
#include "../WiFiDebug.h"
#endif

// Our debugPrint routing logic based on Config.h
#if DEBUG_ENABLED
    bool DEBUG           = true;
    bool debugToSerial   = true;
    #if DEBUG_USE_WIFI
        bool debugToUDP  = true;
    #endif
#elif VERBOSE_PERFORMANCE_ONLY
    bool DEBUG           = false;
    bool debugToSerial   = false;
    bool debugToUDP      = false;
#elif VERBOSE_MODE
    bool DEBUG           = false;
    bool debugToSerial   = true;
    bool debugToUDP      = true;
#elif (VERBOSE_MODE_WIFI_ONLY && DEBUG_USE_WIFI) && VERBOSE_MODE_SERIAL_ONLY 
    bool DEBUG           = false;
    bool debugToSerial   = true;
    bool debugToUDP      = true;
#elif (VERBOSE_MODE_WIFI_ONLY && DEBUG_USE_WIFI)
    bool DEBUG           = false;
    bool debugToSerial   = false;
    bool debugToUDP      = true;
#elif VERBOSE_MODE_SERIAL_ONLY
    bool DEBUG           = false;
    bool debugToSerial   = true;
    bool debugToUDP      = false;
#else
    bool DEBUG           = false;
    bool debugToSerial   = false;
    bool debugToUDP      = false;
#endif

// Temp buffer for Serial ring buffer draining
static char tempBuf[SERIAL_DEBUG_FLUSH_BUFFER_SIZE]; // Big enough for your largest full message (tune as needed)

static volatile uint32_t serialDebugTotalBytes = 0;
static volatile uint32_t serialDebugMsgCount = 0;
static volatile size_t   serialDebugMsgMaxLen = 0;

static volatile uint32_t serialDebugOverflow = 0;
static volatile size_t   serialDebugHighWater = 0;

static SerialDebugMsg serialBuf[SERIAL_RINGBUF_SIZE];
static volatile uint8_t serialHead = 0, serialTail = 0;

inline static bool serialRingFull()  { return ((serialHead + 1) % SERIAL_RINGBUF_SIZE) == serialTail; }
inline static bool serialRingEmpty() { return serialHead == serialTail; }

uint32_t serialDebugGetOverflow(void) { return serialDebugOverflow; }
size_t serialDebugGetHighWater(void) { return serialDebugHighWater; }
size_t serialDebugGetPending(void) { return serialDebugRingPending(); }

float serialDebugAvgMsgLen(void) {
    return (serialDebugMsgCount > 0) ? ((float)serialDebugTotalBytes / serialDebugMsgCount) : 0.0f;
}
size_t serialDebugMaxMsgLen(void) {
    return serialDebugMsgMaxLen;
}

void debugSetOutput(bool toSerial, bool toUDP) {
    debugToSerial = toSerial;
    debugToUDP    = toUDP;
}

void debugPrint(const char* msg) {
    if (debugToSerial) serialDebugPrint(msg);
    #if DEBUG_USE_WIFI
    if (debugToUDP)    wifiDebugPrint(msg);
    #endif
}

void debugPrintln(const char* msg) {
    if (debugToSerial) serialDebugPrintln(msg);
    #if DEBUG_USE_WIFI
    if (debugToUDP)    wifiDebugPrintln(msg);
    #endif
}

void debugPrintf(const char* format, ...) {
    char buf[DEBUGPRINTF_GENERAL_TMP_BUFFER];  // Fixed-size stack buffer (safe)
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    buf[sizeof(buf) - 1] = '\0';  // Force null-termination
    debugPrint(buf);
}

void debugPrintn(const char* msg, size_t len) {
    if (debugToSerial) serialDebugPrintn(msg, len);
    #if DEBUG_USE_WIFI
    if (debugToUDP)    wifiDebugPrintn(msg, len);
    #endif
}

void serialDebugRingPush(const char* msg, size_t len, bool isLastChunk) {
    if (serialRingFull()) {
        serialDebugOverflow++;
        return;
    }
    if (len >= SERIAL_MSG_MAXLEN) len = SERIAL_MSG_MAXLEN - 1;
    memcpy(serialBuf[serialHead].msg, msg, len);
    serialBuf[serialHead].msg[len] = '\0';
    serialBuf[serialHead].len = len;
    serialBuf[serialHead].isLastChunk = isLastChunk;
    serialHead = (serialHead + 1) % SERIAL_RINGBUF_SIZE;

    // High water mark logic
    size_t pending = serialDebugRingPending();
    if (pending > serialDebugHighWater) serialDebugHighWater = pending;

    // --- Added these lines for stats ---
    serialDebugTotalBytes += len;
    serialDebugMsgCount++;
    if (len > serialDebugMsgMaxLen) serialDebugMsgMaxLen = len;   
}

bool serialDebugRingPop(SerialDebugMsg* out) {
    if (serialRingEmpty()) return false;
    *out = serialBuf[serialTail];
    serialTail = (serialTail + 1) % SERIAL_RINGBUF_SIZE;
    return true;
}

size_t serialDebugRingPending() {
    if (serialHead >= serialTail) return serialHead - serialTail;
    return SERIAL_RINGBUF_SIZE - (serialTail - serialHead);
}

// Returns how many slots are available for use
size_t serialDebugRingAvailable() {
    if (serialHead >= serialTail)
        return SERIAL_RINGBUF_SIZE - (serialHead - serialTail) - 1;
    else
        return (serialTail - serialHead) - 1;
}

void serialDebugSendChunked(const char* msg, size_t len) {

    // We send directly via Serial if not using the Serial Debug Ring Buffer
    #if !SERIAL_DEBUG_USE_RINGBUFFER
        if (!isSerialConnected()) return;
        #if (ARDUINO_USB_CDC_ON_BOOT == 1)
        writeToConsole(msg, len);
        #else
        // No serial interface
        #endif
        return;
    #endif    

    const size_t max_data = SERIAL_MSG_MAXLEN - 1;
    size_t temp_pos = 0, temp_len = len, needed = 0;

    // PASS 1: count chunks needed (DO NOT TOUCH pos/l used for actual chunking)
    while (temp_len > 0) {
        size_t take = (temp_len > max_data) ? utf8_chunk_len(msg + temp_pos, max_data) : temp_len;
        if (take == 0) break;
        needed++;
        temp_pos += take;
        temp_len -= take;
    }
    if (serialDebugRingAvailable() < needed) {
        serialDebugOverflow++;
        return;
    }

    // PASS 2: push actual chunks with correct isLastChunk
    size_t pos = 0, l = len, chunk_num = 0;
    while (l > 0) {
        size_t chunk_len = (l > max_data) ? utf8_chunk_len(msg + pos, max_data) : l;
        bool last = (chunk_num == needed - 1);
        serialDebugRingPush(msg + pos, chunk_len, last);
        pos += chunk_len;
        l -= chunk_len;
        chunk_num++;
    }

    if (!mainLoopStarted)
        sendPendingSerial();    
}

void serialDebugPrint(const char* msg) {
    serialDebugSendChunked(msg, strlen(msg));
}

void serialDebugPrintn(const char* msg, size_t len) {
    serialDebugSendChunked(msg, len);
}

void serialDebugPrintf(const char* format, ...) {
    // Max allowable message size for Serial debug (MUST BE LARGER THAN SERIAL_MSG_MAXLEN)
    char serialDebugPrintf_tmp[SERIAL_DEBUG_BUFFER_SIZE]; 
    va_list args;
    va_start(args, format);
    int written = vsnprintf(serialDebugPrintf_tmp, sizeof(serialDebugPrintf_tmp), format, args);
    va_end(args);
    size_t len = (written > 0 && written < (int)sizeof(serialDebugPrintf_tmp)) ? written : (sizeof(serialDebugPrintf_tmp) - 1);
    serialDebugSendChunked(serialDebugPrintf_tmp, len);
}

void serialDebugPrintln(const char* msg) {
    serialDebugPrint(msg);
    serialDebugPrint("\r\n");
}

void dumpSerialRingBuffer() {
#if (ARDUINO_USB_CDC_ON_BOOT == 1)
    char outbuf[1024];
    size_t olen = 0;
    olen += snprintf(outbuf + olen, sizeof(outbuf) - olen, "\n--- Serial Ring Buffer Dump ---\n");

    size_t i = serialTail;
    size_t count = serialDebugRingPending();
    for (size_t n = 0; n < count; ++n) {
        SerialDebugMsg* msg = &serialBuf[i];
        olen += snprintf(outbuf + olen, sizeof(outbuf) - olen,
            "Slot %2u: len=%2u last=%d [", (unsigned)i, (unsigned)msg->len, msg->isLastChunk ? 1 : 0);

        // Print message content
        for (size_t j = 0; j < msg->len && olen < sizeof(outbuf) - 8; ++j) {
            char c = msg->msg[j];
            if (c >= 32 && c <= 126) {
                if (olen < sizeof(outbuf) - 2)
                    outbuf[olen++] = c;
            } else if (c == '\n') {
                olen += snprintf(outbuf + olen, sizeof(outbuf) - olen, "\\n");
            } else if (c == '\r') {
                olen += snprintf(outbuf + olen, sizeof(outbuf) - olen, "\\r");
            } else if (c == '\0') {
                olen += snprintf(outbuf + olen, sizeof(outbuf) - olen, "\\0");
            } else {
                olen += snprintf(outbuf + olen, sizeof(outbuf) - olen, "\\x%02X", (unsigned char)c);
            }
        }
        if (olen < sizeof(outbuf) - 2)
            outbuf[olen++] = ']';
        if (olen < sizeof(outbuf) - 2)
            outbuf[olen++] = '\n';

        i = (i + 1) % SERIAL_RINGBUF_SIZE;
        if (olen > sizeof(outbuf) - 80) break; // avoid overflow, like your UDP logic
    }
    olen += snprintf(outbuf + olen, sizeof(outbuf) - olen, "--- End of Ring Buffer Dump ---\n");

    writeToConsole(outbuf, olen);
#endif
}

void writeToConsole(const char* data, size_t len) {
    if (!isSerialConnected() || !data || len == 0 || !Serial) return;

    // const size_t CHUNK = SERIAL_DEBUG_FLUSH_BUFFER_SIZE;
    const size_t CHUNK = SERIAL_DEBUG_OUTPUT_CHUNK_SIZE;
    size_t offset = 0;

    while (offset < len) {
        size_t toWrite = (len - offset > CHUNK) ? CHUNK : (len - offset);
        Serial.write(data + offset, toWrite);
        // Serial.flush();   // Optional: remove if you want, or leave for extra safety
        // yield();
        offset += toWrite;
    }
}

void sendPendingSerial() {
    if (!isSerialConnected()) return;

    size_t tempPos = 0;

    while (serialDebugRingPending() > 0) { 
        SerialDebugMsg dbgmsg;
        if (!serialDebugRingPop(&dbgmsg)) break;  // Defensive, but probably never false here

        memcpy(tempBuf + tempPos, dbgmsg.msg, dbgmsg.len);
        tempPos += dbgmsg.len;

        if (dbgmsg.isLastChunk) {
            // Write the full reassembled message in one go!
            #if (ARDUINO_USB_CDC_ON_BOOT == 1)
            writeToConsole(tempBuf, tempPos);
            #else
            // Serial is NOT available
            #endif
            tempPos = 0;
        }
    }
}