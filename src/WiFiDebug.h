// WiFiDebug.h

#pragma once

#if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI

#include <WiFi.h>
#include <IPAddress.h>

// Remote DEBUG_REMOTE_IP and DEBUG_REMOTE_PORT of the remote debug console
static const IPAddress DEBUG_REMOTE_IP(DEBUG_CONSOLE_IP_ADDRESS); // Specific IP to send debug messages
static const uint16_t DEBUG_REMOTE_PORT = 4210;

// Not really used but required by AsyncUDP. Its the local port where we listen to any DEBUG messages (only used when NOT using WiFi for DCSBIOS)
static const uint16_t DEBUG_LOCAL_PORT = 4209;

// Remote DCS_COMPUTER_IP_ADDRESS and DCS_REMOTE_PORT where we send UDP messages to DCS. 
// If USE_DCSBIOS_WIFI we join multicast group and listen to 239.255.50.10 port 5010 (like SOCAT does) 
static const IPAddress DCS_REMOTE_IP(DCS_COMPUTER_IP_ADDRESS); // IP for the PC where DCS is installed
static const uint16_t DCS_REMOTE_PORT = 7778;

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