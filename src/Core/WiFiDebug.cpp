// WiFiDebug.cpp - Uses AsyncUDP for non-blocking operation

#include "../Globals.h"

#if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
#include "../DCSBIOSBridge.h"  // Needed if you plan to use DCSBIOS with WiFi
#include "../WiFiDebug.h"
#include <AsyncUDP.h>
static AsyncUDP udp;

IPAddress dcsSourceIP;      // Last source IP seen
static volatile bool dcsSourceIPValid = false;








static char g_deviceName[64] = "UNKNOWN";

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

static void buildDeviceName() {
#if defined(RS485_MASTER_ENABLED) && RS485_MASTER_ENABLED
    snprintf(g_deviceName, sizeof(g_deviceName), "MASTER-%s", LABEL_SET_FULLNAME);
#elif defined(RS485_SLAVE_ENABLED) && RS485_SLAVE_ENABLED
    snprintf(g_deviceName, sizeof(g_deviceName), "SLAVE-%02d-%s", RS485_SLAVE_ADDRESS, LABEL_SET_FULLNAME);
#else
    snprintf(g_deviceName, sizeof(g_deviceName), "%s", LABEL_SET_FULLNAME);
#endif
}








// ═══════════════════════════════════════════════════════════════════════════
// RING BUFFER MODE — Only compiled when WIFI_DEBUG_USE_RINGBUFFER is enabled
// ═══════════════════════════════════════════════════════════════════════════
#if WIFI_DEBUG_USE_RINGBUFFER

static char udpTempBuf[UDP_TMPBUF_SIZE]; // Reassembly buffer for chunked messages
static volatile uint32_t wifiDebugSendOverflow = 0;
static volatile size_t   wifiDebugSendHighWater = 0;
static volatile uint32_t wifiDebugSendTotalBytes = 0;
static volatile uint32_t wifiDebugSendMsgCount = 0;
static volatile size_t   wifiDebugSendMsgMaxLen = 0;
static WifiDebugSendMsg wifiDbgSendBuf[WIFI_DBG_SEND_RINGBUF_SIZE];
static volatile uint8_t wifiDbgSendHead = 0, wifiDbgSendTail = 0;

inline static bool wifiDbgSendRingFull()  { return ((wifiDbgSendHead + 1) % WIFI_DBG_SEND_RINGBUF_SIZE) == wifiDbgSendTail; }
inline static bool wifiDbgSendRingEmpty() { return wifiDbgSendHead == wifiDbgSendTail; }

uint32_t wifiDebugSendGetOverflow(void) { return wifiDebugSendOverflow; }
size_t wifiDebugSendGetHighWater(void) { return wifiDebugSendHighWater; }
float wifiDebugSendAvgMsgLen(void) {
    return (wifiDebugSendMsgCount > 0) ? ((float)wifiDebugSendTotalBytes / wifiDebugSendMsgCount) : 0.0f;
}
size_t wifiDebugSendMaxMsgLen(void) { return wifiDebugSendMsgMaxLen; }

size_t wifiDebugSendRingPending() {
    if (wifiDbgSendHead >= wifiDbgSendTail) return wifiDbgSendHead - wifiDbgSendTail;
    return WIFI_DBG_SEND_RINGBUF_SIZE - (wifiDbgSendTail - wifiDbgSendHead);
}
size_t wifiDebugSendGetPending(void) { return wifiDebugSendRingPending(); }

size_t wifiDebugSendRingAvailable() {
    if (wifiDbgSendHead >= wifiDbgSendTail)
        return WIFI_DBG_SEND_RINGBUF_SIZE - (wifiDbgSendHead - wifiDbgSendTail) - 1;
    else
        return (wifiDbgSendTail - wifiDbgSendHead) - 1;
}

bool wifiDebugSendRingPop(WifiDebugSendMsg* out) {
    if (wifiDbgSendRingEmpty()) return false;
    *out = wifiDbgSendBuf[wifiDbgSendTail];
    wifiDbgSendTail = (wifiDbgSendTail + 1) % WIFI_DBG_SEND_RINGBUF_SIZE;
    return true;
}

void wifiDebugSendRingPush(const char* data, size_t len, bool isLastChunk) {
    if (wifiDbgSendRingFull()) {
        wifiDebugSendOverflow = wifiDebugSendOverflow + 1;
        return;
    }
    if (len >= WIFI_DBG_MSG_MAXLEN) len = WIFI_DBG_MSG_MAXLEN - 1;
    memcpy(wifiDbgSendBuf[wifiDbgSendHead].msg, data, len);
    wifiDbgSendBuf[wifiDbgSendHead].msg[len] = '\0';
    wifiDbgSendBuf[wifiDbgSendHead].len = len;
    wifiDbgSendBuf[wifiDbgSendHead].isLastChunk = isLastChunk;
    wifiDbgSendHead = (wifiDbgSendHead + 1) % WIFI_DBG_SEND_RINGBUF_SIZE;

    // High water mark
    size_t pending = wifiDebugSendRingPending();
    if (pending > wifiDebugSendHighWater) wifiDebugSendHighWater = pending;

    // Stats
    wifiDebugSendTotalBytes += len;
    wifiDebugSendMsgCount = wifiDebugSendMsgCount + 1;
    if (len > wifiDebugSendMsgMaxLen) wifiDebugSendMsgMaxLen = len;
}

void wifiDebugDrainSendBuffer(void) {
    if (WiFi.status() != WL_CONNECTED) return;
    WifiDebugSendMsg dbgmsg;
    size_t tempPos = 0;

    while (wifiDebugSendRingPop(&dbgmsg)) {
        if (tempPos + dbgmsg.len > sizeof(udpTempBuf)) {
            tempPos = 0; // overflow: drop the current message
            wifiDebugSendOverflow = wifiDebugSendOverflow + 1;
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

#else
// ═══════════════════════════════════════════════════════════════════════════
// DIRECT MODE — Stubs when ring buffer is disabled (saves ~1.5KB RAM)
// ═══════════════════════════════════════════════════════════════════════════
uint32_t wifiDebugSendGetOverflow(void) { return 0; }
size_t wifiDebugSendGetHighWater(void) { return 0; }
size_t wifiDebugSendGetPending(void) { return 0; }
float wifiDebugSendAvgMsgLen(void) { return 0.0f; }
size_t wifiDebugSendMaxMsgLen(void) { return 0; }
void wifiDebugDrainSendBuffer(void) { }

#endif // WIFI_DEBUG_USE_RINGBUFFER

// ═══════════════════════════════════════════════════════════════════════════
// COMMON CODE — Always compiled
// ═══════════════════════════════════════════════════════════════════════════

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
            serialDebugPrintf("[DCS] Updated source IP: %s\n", dcsSourceIP.toString().c_str());
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
        serialDebugPrintf("[UDP RECEIVED] %.*s\n",
            (int)packet.length(),
            (const char*)packet.data());
    });

#endif
}

bool tryToSendDcsBiosMessageUDP(const char* msg, const char* arg) {
    if (WiFi.status() != WL_CONNECTED) return false;
    if (!msg || !arg) return false;

    constexpr size_t MAX_MSG = 64;   // max chars allowed
    constexpr size_t MAX_ARG = 32;

    const size_t msgLen = strnlen(msg, MAX_MSG + 1);
    const size_t argLen = strnlen(arg, MAX_ARG + 1);
    if (msgLen == 0 || argLen == 0) return false;        // no empties
    if (msgLen > MAX_MSG || argLen > MAX_ARG) return false; // overflow guard
    // reject whitespace that would break the wire format
    for (size_t i = 0; i < msgLen; ++i) if (msg[i] <= ' ') return false;
    for (size_t i = 0; i < argLen; ++i) if (arg[i] == '\n') return false;

    char buf[MAX_MSG + 1 + MAX_ARG + 2]; // space + '\n'
    size_t len = 0;
    memcpy(buf + len, msg, msgLen); len += msgLen;
    buf[len++] = ' ';
    memcpy(buf + len, arg, argLen); len += argLen;
    buf[len++] = '\n';

    IPAddress ipSnapshot;
    bool haveSource = dcsSourceIPValid;          // snapshot flags first
    if (haveSource) {
        ipSnapshot = dcsSourceIP;                // snapshot IP once
    }
    else {
        if (!ipSnapshot.fromString(DCS_COMPUTER_IP)) ipSnapshot = IPAddress(255, 255, 255, 255);
        // If broadcasting:
        // udp.setBroadcast(true);
    }

    int written = udp.writeTo(reinterpret_cast<const uint8_t*>(buf), len, ipSnapshot, DCS_REMOTE_PORT);
    if (written != static_cast<int>(len)) {
        serialDebugPrintf("[DCS-WIFI] UDP send failed (%d/%u) to %s:%u\n",
            written, static_cast<unsigned>(len),
            ipSnapshot.toString().c_str(), static_cast<unsigned>(DCS_REMOTE_PORT));
        return false;
    }

	// delay(1); // tiny delay to avoid flooding
	// delayMicroseconds(100); / / tiny delay to avoid flooding
	yield(); // yield to allow background UDP processing
    
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
    int wifiAttempts = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        serialDebugPrint(".");
        if (++wifiAttempts > 40) {  // 20 second timeout
            serialDebugPrintln("\nWiFi connection failed - continuing without WiFi");
            return;
        }
    }

    IPAddress ip = WiFi.localIP();
    char ipbuf[24];
    snprintf(ipbuf, sizeof(ipbuf), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    serialDebugPrintf(" '%s' Connected to WiFi network %s with IP %s\n", USB_PRODUCT, WIFI_SSID, ipbuf);
    wifiDebugInit(DEBUG_LOCAL_PORT);
    delay(100);

	// Send registration message
    buildDeviceName();
    wifiDebugPrintf("@@REGISTER:%s\n", g_deviceName);

    wifiDebugPrintf(" '%s' Connected to WiFi network %s with IP %s\n", USB_PRODUCT, WIFI_SSID, ipbuf);
}

// --------- FORMATTED/LINE-BASED DEBUG PRINTS ---------

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

void wifiDebugSendChunked(const char* data, size_t len) {

    // Direct send when ring buffer disabled — no reassembly overhead
    #if !WIFI_DEBUG_USE_RINGBUFFER
        wifiDebugSendRaw(data, len);   
        return;
    #else
    // Ring buffer mode — chunk and queue for deferred send
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
        wifiDebugSendOverflow = wifiDebugSendOverflow + 1;
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
    #endif
}

void wifiDebugPrint(const char* msg) {
    wifiDebugSendChunked(msg, strlen(msg));
}

void wifiDebugPrintn(const char* msg, size_t len) {
    wifiDebugSendChunked(msg, len);
}

void wifiDebugPrintf(const char* format, ...) {
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
    wifiDebugSendChunked("\n", 1);
}

#endif // DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
