// RingBuffer.cpp - Implements a simple ring buffer for transport
// --- DCS Stream UDP In Ring Buffer (Host → Device) ---

#include "Arduino.h"
#include "RingBuffer.h"

// Buffer and indices
static DcsUdpRingMsg dcsUdpRing[DCS_USB_RINGBUF_SIZE];
static volatile uint8_t dcsUdpHead = 0, dcsUdpTail = 0;

// Stats
static volatile uint32_t dcsUdpRecvTotalBytes = 0;
static volatile uint32_t dcsUdpRecvMsgCount = 0;
static volatile size_t   dcsUdpRecvMsgMaxLen = 0;
static volatile uint32_t dcsUdpRecvOverflow = 0;
static volatile size_t   dcsUdpRecvHighWater = 0;

inline static bool dcsUdpRingFull()  { return ((dcsUdpHead + 1) % DCS_USB_RINGBUF_SIZE) == dcsUdpTail; }
inline static bool dcsUdpRingEmpty() { return dcsUdpHead == dcsUdpTail; }

uint32_t dcsUdpRecvGetOverflow(void)      { return dcsUdpRecvOverflow; }
size_t   dcsUdpRecvGetHighWater(void)     { return dcsUdpRecvHighWater; }
size_t   dcsUdpRecvGetPending(void)       { return dcsUdpRingbufPending(); }
float    dcsUdpRecvAvgMsgLen(void)        { return (dcsUdpRecvMsgCount > 0) ? ((float)dcsUdpRecvTotalBytes / dcsUdpRecvMsgCount) : 0.0f; }
size_t   dcsUdpRecvMaxMsgLen(void)        { return dcsUdpRecvMsgMaxLen; }

size_t dcsUdpRingbufPending() {
    if (dcsUdpHead >= dcsUdpTail) return dcsUdpHead - dcsUdpTail;
    return DCS_USB_RINGBUF_SIZE - (dcsUdpTail - dcsUdpHead);
}

size_t dcsUdpRingbufAvailable() {
    if (dcsUdpHead >= dcsUdpTail)
        return DCS_USB_RINGBUF_SIZE - (dcsUdpHead - dcsUdpTail) - 1;
    else
        return (dcsUdpTail - dcsUdpHead) - 1;
}

bool dcsUdpRingbufPop(DcsUdpRingMsg* out) {
    if (dcsUdpRingEmpty()) return false;
    *out = dcsUdpRing[dcsUdpTail];
    dcsUdpTail = (dcsUdpTail + 1) % DCS_USB_RINGBUF_SIZE;
    return true;
}

void dcsUdpRingbufPush(const uint8_t* data, size_t len, bool isLastChunk) {
    if (dcsUdpRingFull()) {
        dcsUdpRecvOverflow++;
        // debugPrintln("❌ [RING BUFFER] Ring buffer is FULL, increase DCS_USB_RINGBUF_SIZE");
        return;
    }
    if (len > DCS_USB_PACKET_MAXLEN) len = DCS_USB_PACKET_MAXLEN;
    memcpy(dcsUdpRing[dcsUdpHead].data, data, len);
    dcsUdpRing[dcsUdpHead].len = len;
    dcsUdpRing[dcsUdpHead].isLastChunk = isLastChunk;
    dcsUdpHead = (dcsUdpHead + 1) % DCS_USB_RINGBUF_SIZE;

    size_t pending = dcsUdpRingbufPending();
    if (pending > dcsUdpRecvHighWater) dcsUdpRecvHighWater = pending;

    dcsUdpRecvTotalBytes += len;
    dcsUdpRecvMsgCount++;
    if (len > dcsUdpRecvMsgMaxLen) dcsUdpRecvMsgMaxLen = len;    
}

void dcsUdpRingbufPushChunked(const uint8_t* data, size_t len) {
    const size_t max_data = DCS_USB_PACKET_MAXLEN;
    size_t needed = (len + max_data - 1) / max_data;
    if (dcsUdpRingbufAvailable() < needed) {
        dcsUdpRecvOverflow++;
        // debugPrintln("❌ [RING BUFFER] Available space was less than required, increase DCS_USB_RINGBUF_SIZE");
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
        // debugPrintln("❌ [RING BUFFER] Outgoing message overflow! increase DCS_USB_RINGBUF_SIZE");
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
    const size_t max_data = DCS_USB_PACKET_MAXLEN;
    size_t needed = (len + max_data - 1) / max_data;
    if (dcsRawUsbOutRingbufAvailable() < needed) {
        dcsRawUsbOutOverflow++;
        // debugPrintln("❌ [RING BUFFER] Outgoing message queue would overflow, skipping. Increase DCS_USB_RINGBUF_SIZE");
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
}