// WiFiDebug.h

#pragma once

#if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI

#include <WiFi.h>
#include <IPAddress.h>

#define DCS_COMPUTER_IP "255.255.255.255" 

static const IPAddress DEBUG_REMOTE_IP(255,255,255,255);
static const uint16_t DEBUG_REMOTE_PORT = 4210;

static const IPAddress DCS_REMOTE_IP(255,255,255,255);
static const uint16_t DCS_REMOTE_PORT = 7778;

// Not really used but required by AsyncUDP. Its the local port where we listen to any DEBUG messages (only used when NOT using WiFi for DCSBIOS)
static const uint16_t DEBUG_LOCAL_PORT = 4209;

// --- UDP Send stats accessors ---
uint32_t wifiDebugSendGetOverflow(void);
size_t   wifiDebugSendGetHighWater(void);
size_t   wifiDebugSendGetPending(void);
float    wifiDebugSendAvgMsgLen();
size_t   wifiDebugSendMaxMsgLen();

// --- UDP Send buffer struct ---
typedef struct {
    char msg[WIFI_DBG_MSG_MAXLEN];
    size_t len;
    bool isLastChunk;
} WifiDebugSendMsg;

// --- Send buffer accessors ---
void   wifiDebugSendRingPush(const char* data, size_t len, bool isLastChunk);
bool   wifiDebugSendRingPop(WifiDebugSendMsg* out);
size_t wifiDebugSendRingPending(void);

// --- Central UDP send logic ---
void wifiDebugSendChunked(const char* data, size_t len);
void wifiDebugDrainSendBuffer(void);

// --- WiFi / Debug API ---
void scanNetworks();
void wifiDebugInit(uint16_t localPort = DEBUG_REMOTE_PORT);
void wifiDebugPrint(const char* msg);
void wifiDebugPrintf(const char* format, ...);
void wifiDebugPrintln(const char* msg);
void wifiDebugPrintn(const char* msg, size_t len);
void wifi_setup();

// --- DCS BIOS UDP Send Message/Command ---
bool tryToSendDcsBiosMessageUDP(const char* msg, const char* arg);

// To call raw UDP
void wifiDebugSendRaw(const char* data, size_t len);

#endif // DEBUG_USE_WIFI || USE_DCSBIOS_WIFI