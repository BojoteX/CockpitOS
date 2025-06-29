// WiFiDebug.cpp - Uses AsyncUDP for non-blocking operation

#include "../Globals.h"

#if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
#include "../DCSBIOSBridge.h"  // Needed if you plan to use DCSBIOS with WiFi
#include "../WiFiDebug.h"
#include <AsyncUDP.h>
static AsyncUDP udp;

IPAddress dcsSourceIP;      // Last source IP seen
bool dcsSourceIPValid = false;

static char udpTempBuf[UDP_TMPBUF_SIZE]; // Max Size of UDP Debug Send Packet
static volatile uint32_t wifiDebugSendOverflow = 0;
static volatile size_t   wifiDebugSendHighWater = 0;
uint32_t wifiDebugSendGetOverflow(void) { return wifiDebugSendOverflow; }
size_t wifiDebugSendGetHighWater(void) { return wifiDebugSendHighWater; }
size_t wifiDebugSendGetPending(void) { return wifiDebugSendRingPending(); }
static volatile uint32_t wifiDebugSendTotalBytes = 0;
static volatile uint32_t wifiDebugSendMsgCount = 0;
static volatile size_t   wifiDebugSendMsgMaxLen = 0;
static WifiDebugSendMsg wifiDbgSendBuf[WIFI_DBG_SEND_RINGBUF_SIZE];
static volatile uint8_t wifiDbgSendHead = 0, wifiDbgSendTail = 0;
inline static bool wifiDbgSendRingFull()  { return ((wifiDbgSendHead + 1) % WIFI_DBG_SEND_RINGBUF_SIZE) == wifiDbgSendTail; }
inline static bool wifiDbgSendRingEmpty() { return wifiDbgSendHead == wifiDbgSendTail; }

float wifiDebugSendAvgMsgLen(void) {
    return (wifiDebugSendMsgCount > 0) ? ((float)wifiDebugSendTotalBytes / wifiDebugSendMsgCount) : 0.0f;
}
size_t wifiDebugSendMaxMsgLen(void) {
    return wifiDebugSendMsgMaxLen;
}

// ---------- NETWORK UTILITIES / BOILERPLATE ----------

void scanNetworks() {
    WiFi.mode(WIFI_STA);
    serialDebugPrintln("\nScanning for Wi-Fi networks…");

    int n = WiFi.scanNetworks();
    if (n == 0) {
        serialDebugPrintln("  ► No networks found");
        
    } else {
        for (int i = 0; i < n; i++) {
            char ssidBuf[33];
            WiFi.SSID(i).toCharArray(ssidBuf, sizeof(ssidBuf));
            int  rssi = WiFi.RSSI(i);
            bool sec  = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);

            serialDebugPrintf("  %2d: %-32s %4ddBm  %s\n",
                          i + 1,
                          ssidBuf,
                          rssi,
                          sec ? "🔒 Secured" : "🔓 Open");
            delay(5);
        }
    }
    WiFi.scanDelete();
    serialDebugPrintln("");
}

void wifiDebugInit(uint16_t localPort) {
    if (WiFi.status() != WL_CONNECTED) return;

#if USE_DCSBIOS_WIFI
    udp.listenMulticast(IPAddress(239,255,50,10), 5010);

    udp.onPacket([](AsyncUDPPacket packet) {

        // Update DCS Source IP if new
        if (!dcsSourceIPValid || dcsSourceIP != packet.remoteIP()) {
            dcsSourceIP = packet.remoteIP();
            dcsSourceIPValid = true;
            debugPrintf("[DCS] Updated source IP: %s\n", dcsSourceIP.toString().c_str());
        }

        // Don't do anything unless we start our main loop
        if(!mainLoopStarted) return;

        #if DEBUG_PERFORMANCE
        beginProfiling(PERF_WIFI_DCSBIOS);
        #endif

        #if DCS_USE_RINGBUFFER
            dcsUdpRingbufPushChunked(packet.data(), packet.length());
        #else
            parseDcsBiosUdpPacket(packet.data(), packet.length());
        #endif

        #if DEBUG_PERFORMANCE
        endProfiling(PERF_WIFI_DCSBIOS);
        #endif
    });
#else
    udp.listen(localPort);  // (required, even if we don't care about receiving)

    udp.onPacket([](AsyncUDPPacket packet) {
        debugPrintf("[UDP] %s\n", packet.data());
    });
#endif
}

bool tryToSendDcsBiosMessageUDP(const char* msg, const char* arg) {
    if (WiFi.status() != WL_CONNECTED) return false;

    if (!msg || !arg) return false;

    constexpr size_t maxMsgLen = 64;
    constexpr size_t maxArgLen = 32;

    size_t msgLen = strnlen(msg, maxMsgLen);
    size_t argLen = strnlen(arg, maxArgLen);

    if (msgLen == maxMsgLen || argLen == maxArgLen) return false;

    char buf[maxMsgLen + 1 + maxArgLen + 3]; // "MSG ARG\r\n\0"
    size_t len = 0;
    memcpy(buf, msg, msgLen);
    buf[msgLen] = ' ';
    memcpy(buf + msgLen + 1, arg, argLen);
    len = msgLen + 1 + argLen;
    buf[len++] = '\r';
    buf[len++] = '\n';
    buf[len] = '\0'; // Not needed for UDP, but safe

    /*
    // Direct send to DCS port (not debug)
    udp.writeTo((const uint8_t*)buf, len, DCS_REMOTE_IP, DCS_REMOTE_PORT);
    */

	// Direct send to DCS port (not debug) with DCS source IP validation and automatic fallback
    IPAddress targetIP = dcsSourceIPValid ? dcsSourceIP : IPAddress(DCS_REMOTE_IP); // macro or config
    udp.writeTo((const uint8_t*)buf, len, targetIP, DCS_REMOTE_PORT);


    return true;
}

void wifi_setup() {
    WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);     
  #if SCAN_WIFI_NETWORKS
    scanNetworks();
  #endif
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    serialDebugPrint("Connecting...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        serialDebugPrint(".");
    }

    IPAddress ip = WiFi.localIP();
    char ipbuf[24];
    snprintf(ipbuf, sizeof(ipbuf), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    serialDebugPrintf(" Connected to WiFi network %s with IP %s\n", WIFI_SSID, ipbuf);
    wifiDebugInit(DEBUG_LOCAL_PORT);
    delay(100);
    wifiDebugPrintf(" Connected to WiFi network %s with IP %s\n", WIFI_SSID, ipbuf);
}

// --------- FORMATTED/LINE-BASED DEBUG PRINTS ---------

size_t wifiDebugSendRingPending() {
    if (wifiDbgSendHead >= wifiDbgSendTail) return wifiDbgSendHead - wifiDbgSendTail;
    return WIFI_DBG_SEND_RINGBUF_SIZE - (wifiDbgSendTail - wifiDbgSendHead);
}

size_t wifiDebugSendRingAvailable() {
    if (wifiDbgSendHead >= wifiDbgSendTail)
        return WIFI_DBG_SEND_RINGBUF_SIZE - (wifiDbgSendHead - wifiDbgSendTail) - 1;
    else
        return (wifiDbgSendTail - wifiDbgSendHead) - 1;
}

void wifiDebugSendChunked(const char* data, size_t len) {

    // We send directly via UDP if not using the UDP Send Debug Ring Buffer
    #if !WIFI_DEBUG_USE_RINGBUFFER
        wifiDebugSendRaw(data, len);   
        return;
    #endif

    size_t max_data = WIFI_DBG_MSG_MAXLEN - 1;
    // --- PASS 1: Count chunks needed, UTF-8-safe
    size_t temp_pos = 0, temp_len = len, needed = 0;
    while (temp_len > 0) {
        size_t take = (temp_len > max_data) ? utf8_chunk_len(data + temp_pos, max_data) : temp_len;
        if (take == 0) break;
        needed++;
        temp_pos += take;
        temp_len -= take;
    }
    if (wifiDebugSendRingAvailable() < needed) {
        wifiDebugSendOverflow++;
        return; // Drop message, never partial log
    }

    // --- PASS 2: Actually split & push, UTF-8-safe
    size_t pos = 0, l = len, chunk_num = 0;
    while (l > 0) {
        size_t chunk_len = (l > max_data) ? utf8_chunk_len(data + pos, max_data) : l;
        bool last = (chunk_num == needed - 1);
        wifiDebugSendRingPush(data + pos, chunk_len, last);
        pos += chunk_len;
        l -= chunk_len;
        chunk_num++;
    }

    if(!mainLoopStarted)
        wifiDebugDrainSendBuffer();
}

// UDP Send debug Ring Buffer implementation
bool wifiDebugSendRingPop(WifiDebugSendMsg* out) {
    if (wifiDbgSendRingEmpty()) return false;
    *out = wifiDbgSendBuf[wifiDbgSendTail];
    wifiDbgSendTail = (wifiDbgSendTail + 1) % WIFI_DBG_SEND_RINGBUF_SIZE;
    return true;
}

// Then in your drain function:
void wifiDebugDrainSendBuffer(void) {
    if (WiFi.status() != WL_CONNECTED) return;
    WifiDebugSendMsg dbgmsg;
    size_t tempPos = 0;

    while (wifiDebugSendRingPop(&dbgmsg)) {
        if (tempPos + dbgmsg.len > sizeof(udpTempBuf)) {
            tempPos = 0; // overflow: drop the current message
            wifiDebugSendOverflow++;
            continue;
        }
        memcpy(udpTempBuf + tempPos, dbgmsg.msg, dbgmsg.len);
        tempPos += dbgmsg.len;
        if (dbgmsg.isLastChunk) {
            wifiDebugSendRaw(udpTempBuf, tempPos);
            tempPos = 0;
        }
    }
}

void wifiDebugSendRingPush(const char* data, size_t len, bool isLastChunk) {
    if (wifiDbgSendRingFull()) {
        wifiDebugSendOverflow++;
        return;
    }
    if (len >= WIFI_DBG_MSG_MAXLEN) len = WIFI_DBG_MSG_MAXLEN - 1;
    memcpy(wifiDbgSendBuf[wifiDbgSendHead].msg, data, len);
    wifiDbgSendBuf[wifiDbgSendHead].msg[len] = '\0';
    wifiDbgSendBuf[wifiDbgSendHead].len = len;
    wifiDbgSendBuf[wifiDbgSendHead].isLastChunk = isLastChunk; // <---
    wifiDbgSendHead = (wifiDbgSendHead + 1) % WIFI_DBG_SEND_RINGBUF_SIZE;

    // High water mark
    size_t pending = wifiDebugSendRingPending();
    if (pending > wifiDebugSendHighWater) wifiDebugSendHighWater = pending;

    // --- Added these lines for stats ---
    wifiDebugSendTotalBytes += len;
    wifiDebugSendMsgCount++;
    if (len > wifiDebugSendMsgMaxLen) wifiDebugSendMsgMaxLen = len;    
}

void wifiDebugPrint(const char* msg) {
    wifiDebugSendChunked(msg, strlen(msg));
}
void wifiDebugPrintn(const char* msg, size_t len) {
    wifiDebugSendChunked(msg, len);
}

void wifiDebugPrintf(const char* format, ...) {
    // Max allowable message size for WiFi debug (MUST BE LARGER THAN WIFI_DBG_MSG_MAXLEN)
    char wifiDebugPrintf_buf[WIFI_DEBUG_BUFFER_SIZE];    
    va_list args;
    va_start(args, format);
    int written = vsnprintf(wifiDebugPrintf_buf, sizeof(wifiDebugPrintf_buf), format, args);
    va_end(args);
    size_t len = (written > 0 && written < (int)sizeof(wifiDebugPrintf_buf)) ? written : (sizeof(wifiDebugPrintf_buf) - 1);
    wifiDebugSendChunked(wifiDebugPrintf_buf, len);
}

void wifiDebugPrintln(const char* msg) {
    wifiDebugSendChunked(msg, strlen(msg));
}

void wifiDebugSendRaw(const char* data, size_t len) {
    if (WiFi.status() != WL_CONNECTED) return;

    const size_t CHUNK = DCS_UDP_MAX_REASSEMBLED;
    size_t offset = 0;
    while (offset < len) {
        size_t toSend = (len - offset > CHUNK) ? CHUNK : (len - offset);
        udp.writeTo((const uint8_t*)(data + offset), toSend, DEBUG_REMOTE_IP, DEBUG_REMOTE_PORT);
        offset += toSend;
    }
}

#endif // DEBUG_USE_WIFI || USE_DCSBIOS_WIFI