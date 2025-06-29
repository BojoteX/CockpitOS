// debugPrint.h

#pragma once

void writeToConsole(const char* data, size_t len);
void dumpSerialRingBuffer();

static inline size_t utf8_chunk_len(const char* s, size_t max_bytes) {
    if (max_bytes == 0) return 0;
    size_t len = max_bytes;
    while (len > 0) {
        unsigned char c = (unsigned char)s[len-1];
        if ((c & 0xC0) != 0x80) break; // Not a continuation byte → valid boundary
        len--;
    }
    return len;
}

typedef struct {
    char msg[SERIAL_MSG_MAXLEN];
    size_t len;
    bool isLastChunk; 
} SerialDebugMsg;

void serialDebugRingPush(const char* msg, size_t len);
bool serialDebugRingPop(SerialDebugMsg* out);
size_t serialDebugRingPending();

void debugSetOutput(bool toSerial, bool toUDP);
void debugPrint(const char* msg);
void debugPrintln(const char* msg = "");
void debugPrintf(const char* format, ...);
void debugPrintn(const char* msg, size_t len);
void serialDebugPrintf(const char* format, ...);
void sendDebug(const char* msg);
void serialDebugPrint(const char* msg);
void serialDebugPrintln(const char* msg = "");
void serialDebugPrintn(const char* msg, size_t len);

float serialDebugAvgMsgLen();
size_t serialDebugMaxMsgLen();

uint32_t serialDebugGetOverflow();
size_t   serialDebugGetHighWater();
size_t   serialDebugGetPending();

void sendPendingSerial();