// PerfMonitor.cpp

#include "../Globals.h"     

// Find the #include for Arduino.h or before your printTaskList()
#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
// Core 3.x and later: xCoreID is present
#define HAS_XCOREID_FIELD 1
#define HAS_UXTASKGETSYSTEMSTATE 1
#else
#define HAS_XCOREID_FIELD 0
#define HAS_UXTASKGETSYSTEMSTATE 0
#endif

#if DEBUG_PERFORMANCE

#if (ARDUINO_USB_CDC_ON_BOOT == 1)
#include "tusb.h"
#endif

#if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
  #include "../WiFiDebug.h"
#endif 

ProfAccum perfTable[PERF_LABEL_COUNT];
#if !PERFORMANCE_SNAPSHOT_INTERVAL_SECONDS
#undef PERFORMANCE_SNAPSHOT_INTERVAL_SECONDS 
#define PERFORMANCE_SNAPSHOT_INTERVAL_SECONDS 60 // Set to 60 seconds if set to 0 in Config.h
#endif
#define PERFORMANCE_SNAPSHOT_INTERVAL_MS (PERFORMANCE_SNAPSHOT_INTERVAL_SECONDS * 1000UL)

static char perfDebugTmpBuf[PERF_TMPBUF_SIZE];
static size_t perfDebugTmpLen = 0;   

// ——— Monitoring state ———
static unsigned long _lastReportMs = 0;
static unsigned long _lastLoopUs   = 0;
static uint64_t       _busyUsAccum = 0;

// One-time bad-reset alert guard
static bool _alertShown = false;

// === Helpers ===
inline void perfDebugPrint(const char* msg) {

#if VERBOSE_MODE
    #if DEBUG_USE_WIFI
    wifiDebugPrint(msg);
    #endif
    serialDebugPrint(msg); 
#else
    #if (DEBUG_USE_WIFI && VERBOSE_MODE_WIFI_ONLY)
        wifiDebugPrint(msg);
    #endif
    #if VERBOSE_MODE_SERIAL_ONLY
        serialDebugPrint(msg); 
    #endif
#endif
}

inline void perfDebugPrintln(const char* msg = "") {
#if VERBOSE_MODE
    #if DEBUG_USE_WIFI
    wifiDebugPrintln(msg);
    #endif
    serialDebugPrintln(msg);
#else
    #if (DEBUG_USE_WIFI && VERBOSE_MODE_WIFI_ONLY)
        wifiDebugPrintln(msg);
    #endif
    #if VERBOSE_MODE_SERIAL_ONLY
        serialDebugPrintln(msg);
    #endif
#endif
}

inline void perfDebugPrintf(const char* format, ...) {
    char buf[PERF_TMPBUF_SIZE];
    va_list args;
    va_start(args, format);
    int written = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    size_t len = (written >= 0 && written < (int)sizeof(buf)) ? (size_t)written : (sizeof(buf) - 1);
#if VERBOSE_MODE
    #if DEBUG_USE_WIFI
    wifiDebugPrintn(buf, len);
    #endif
    serialDebugPrintn(buf, len);
#else
    #if (DEBUG_USE_WIFI && VERBOSE_MODE_WIFI_ONLY)
        wifiDebugPrintn(buf, len);
    #endif
    #if VERBOSE_MODE_SERIAL_ONLY
        serialDebugPrintn(buf, len);
    #endif
#endif
}

inline void perfDebugPrintn(const char* msg, size_t len) {
#if VERBOSE_MODE
    #if DEBUG_USE_WIFI
    wifiDebugPrintn(msg, len);
    #endif
    serialDebugPrintn(msg, len);
#else
    #if (DEBUG_USE_WIFI && VERBOSE_MODE_WIFI_ONLY)
        wifiDebugPrintn(msg, len);
    #endif
    #if VERBOSE_MODE_SERIAL_ONLY
        serialDebugPrintn(msg, len);
    #endif
#endif
}

void flushAllprintsAbove() {

    // Flush all Pending UDP debug messages via UDP
    #if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
    wifiDebugDrainSendBuffer(); // Sending pending WiFi Debug messages
    #endif

    // Flush all Pending Serial debug messages via CDCSerial (if enabled)
    sendPendingSerial();

    if (perfDebugTmpLen == 0) return;

#if (VERBOSE_MODE_WIFI_ONLY && DEBUG_USE_WIFI) || (DEBUG_USE_WIFI && VERBOSE_MODE)
    size_t start = 0;
    while (start < perfDebugTmpLen) {
        // Find next newline (or end)
        size_t end = start;
        while (end < perfDebugTmpLen && perfDebugTmpBuf[end] != '\n') ++end;
        // Include the newline if present
        size_t lineLen = (end < perfDebugTmpLen) ? (end - start + 1) : (end - start);
        wifiDebugSendChunked(perfDebugTmpBuf + start, lineLen);
        start = end + 1;
    }
#endif

    // --- Serial: chunk by SERIAL_MSG_MAXLEN, UTF-8 safe, line by line ---
#if (VERBOSE_MODE_SERIAL_ONLY) || (VERBOSE_MODE)
    size_t start_s = 0;
    while (start_s < perfDebugTmpLen) {
        // Find next newline (or end)
        size_t end = start_s;
        while (end < perfDebugTmpLen && perfDebugTmpBuf[end] != '\n') ++end;
        // Include the newline if present
        size_t lineLen = (end < perfDebugTmpLen) ? (end - start_s + 1) : (end - start_s);
        size_t linePos = 0;
        while (lineLen > 0) {
            size_t max_chunk = (lineLen > SERIAL_MSG_MAXLEN-1) ? (SERIAL_MSG_MAXLEN-1) : lineLen;
            size_t chunk = utf8_chunk_len(perfDebugTmpBuf + start_s + linePos, max_chunk);
            if (chunk == 0) chunk = max_chunk; // Failsafe
            char outbuf[SERIAL_MSG_MAXLEN];
            memcpy(outbuf, perfDebugTmpBuf + start_s + linePos, chunk);
            outbuf[chunk] = '\0';
            serialDebugPrint(outbuf);
            linePos += chunk;
            lineLen -= chunk;
        }
        start_s = end + 1;
    }
#endif

    perfDebugTmpLen = 0;
    perfDebugTmpBuf[0] = '\0';
}

void appendOnly_perfDebugPrint(const char* msg) {
    size_t len = strlen(msg);
    // If not enough space, flush and start again
    if (perfDebugTmpLen + len >= PERF_TMPBUF_SIZE) {
        flushAllprintsAbove();
    }
    // If still too large (single message exceeds buffer), truncate
    size_t space = PERF_TMPBUF_SIZE - perfDebugTmpLen - 1; // leave room for '\0'
    if (len > space) len = space;
    memcpy(perfDebugTmpBuf + perfDebugTmpLen, msg, len);
    perfDebugTmpLen += len;
    perfDebugTmpBuf[perfDebugTmpLen] = '\0';
}

void appendOnly_perfDebugPrintf(const char* format, ...) {
    size_t space = PERF_TMPBUF_SIZE - perfDebugTmpLen - 1; // leave room for '\0'
    // If no space, flush first
    if (space == 0) {
        flushAllprintsAbove();
        space = PERF_TMPBUF_SIZE - perfDebugTmpLen - 1;
    }

    va_list args;
    va_start(args, format);
    int written = vsnprintf(perfDebugTmpBuf + perfDebugTmpLen, space + 1, format, args); // +1 for '\0'
    va_end(args);

    // If output doesn't fit, flush and try again (but only once to avoid infinite loop)
    if (written > (int)space) {
        flushAllprintsAbove();
        space = PERF_TMPBUF_SIZE - perfDebugTmpLen - 1;
        va_start(args, format);
        written = vsnprintf(perfDebugTmpBuf + perfDebugTmpLen, space + 1, format, args);
        va_end(args);
        if (written > (int)space) written = (int)space; // truncate if single line too big
    }
    if (written > 0) {
        perfDebugTmpLen += (size_t)written;
        if (perfDebugTmpLen >= PERF_TMPBUF_SIZE) perfDebugTmpLen = PERF_TMPBUF_SIZE - 1;
        perfDebugTmpBuf[perfDebugTmpLen] = '\0';
    }
}


void appendOnly_perfDebugPrintln(const char* msg = "") {
    appendOnly_perfDebugPrint(msg);
    appendOnly_perfDebugPrint("\r\n");
}

void printTaskList() {
#if HAS_UXTASKGETSYSTEMSTATE
    constexpr size_t MAX_TASKS = 32;
    TaskStatus_t taskStatusArray[MAX_TASKS];
    UBaseType_t taskCount = uxTaskGetSystemState(taskStatusArray, MAX_TASKS, nullptr);

    appendOnly_perfDebugPrintln("\r\n📋 Detailed FreeRTOS Task List:");
    appendOnly_perfDebugPrintln("---------------------------------------------------------------------------------------");
    appendOnly_perfDebugPrintln("Name              State   Prio  Core  StackFree  Handle     ID");
    appendOnly_perfDebugPrintln("---------------------------------------------------------------------------------------");

    for (UBaseType_t i = 0; i < taskCount; ++i) {
        const TaskStatus_t& t = taskStatusArray[i];

        const char* stateStr = "UNKNOWN";
        switch (t.eCurrentState) {
        case eRunning:   stateStr = "RUN";   break;
        case eReady:     stateStr = "READY"; break;
        case eBlocked:   stateStr = "BLOCK"; break;
        case eSuspended: stateStr = "SUSP";  break;
        case eDeleted:   stateStr = "DEL";   break;
        default: break;
        }

        char line[TASKLIST_LINE_GENERAL_TMP_BUFFER];
        snprintf(line, sizeof(line),
            "%-17s %-6s %5u    %1d    %7u  0x%08X  %2u",
            t.pcTaskName,
            stateStr,
            (unsigned)t.uxCurrentPriority,
#if HAS_XCOREID_FIELD
            (int)t.xCoreID,
#else
            0,
#endif
            (unsigned)t.usStackHighWaterMark * sizeof(StackType_t),
            (unsigned int)(uintptr_t)t.xHandle,
            (unsigned)t.xTaskNumber
        );

        appendOnly_perfDebugPrintln(line);
    }
    flushAllprintsAbove();
#else
    appendOnly_perfDebugPrintln("\r\n📋 Detailed FreeRTOS Task List: Not available (ESP32 Arduino Core 2.x).");
#endif
}

void logCrashDetailIfAny() {
    esp_reset_reason_t reason = esp_reset_reason();
    if (reason == ESP_RST_PANIC) {
        #if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
        wifiDebugPrintln("🧠 Backtrace not available — use UART for more detail.");
        #else
        serialDebugPrintln("🧠 Backtrace not available — use UART for more detail.");
        #endif
    }
}

// True if this reset should trigger an alert
static bool _isBadReset(esp_reset_reason_t r) {
    return (r == ESP_RST_INT_WDT)   ||
           (r == ESP_RST_TASK_WDT)  ||
           (r == ESP_RST_WDT)       ||
           (r == ESP_RST_BROWNOUT)  ||
           (r == ESP_RST_PANIC);
}

// Convert reset codes to a human string
static const char* _resetReasonToString(esp_reset_reason_t r) {
    switch(r) {
      case ESP_RST_UNKNOWN:   return "Unknown";
      case ESP_RST_POWERON:   return "Power-on";
      case ESP_RST_EXT:       return "External";
      case ESP_RST_SW:        return "Software";
      case ESP_RST_PANIC:     return "Panic";
      case ESP_RST_INT_WDT:   return "IntWatchdog";
      case ESP_RST_TASK_WDT:  return "TaskWatchdog";
      case ESP_RST_WDT:       return "OtherWatchdog";
      case ESP_RST_DEEPSLEEP: return "DeepSleepWake";
      case ESP_RST_BROWNOUT:  return "Brownout";
      case ESP_RST_SDIO:      return "SDIO";
      default:                return "??";
    }
}

// === API ===
void initPerfMonitor() {
    // esp_log_level_set("*", ESP_LOG_VERBOSE);
    if (!_alertShown) {
        _alertShown = true;
        auto reason = esp_reset_reason();
        if (_isBadReset(reason)) {

            #if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
            wifiDebugPrintln("\n----- ALERT: Unexpected Reset -----");
            wifiDebugPrintf("Last reset cause: %s (%d)\n\n", _resetReasonToString(reason), reason);
            #else
            serialDebugPrintln("\n----- ALERT: Unexpected Reset -----");
            serialDebugPrintf("Last reset cause: %s (%d)\n\n", _resetReasonToString(reason), reason);
            #endif

            logCrashDetailIfAny();
            perfMonitorUpdate();

            pinMode(LED_BUILTIN, OUTPUT);
            unsigned long start = millis();
            while (millis() - start < 30000UL) {
                digitalWrite(LED_BUILTIN, HIGH);
                delay(500);
                digitalWrite(LED_BUILTIN, LOW);
                delay(500);
            }

            #if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
            wifiDebugPrintln("\nResuming normal operation...\n");
            #else
            serialDebugPrintln("\nResuming normal operation...\n");
            #endif
        }
    }

    _lastReportMs = millis();
    _lastLoopUs   = micros();
}

void beginProfiling(PerfLabel label) {
    perfTable[label].startUs = micros();
}

void endProfiling(PerfLabel label) {
    uint32_t elapsed = micros() - perfTable[label].startUs;
    perfTable[label].sumUs += elapsed;
    perfTable[label].cnt   += 1;
}

void perfMonitorUpdate() {
    unsigned long nowMs = millis();
    if (nowMs - _lastReportMs < PERFORMANCE_SNAPSHOT_INTERVAL_MS) return;
    _lastReportMs = nowMs;

    appendOnly_perfDebugPrintln();
    appendOnly_perfDebugPrintln("+-------------------- PERFORMANCE SNAPSHOT ----------------------+");
    appendOnly_perfDebugPrintln("🔍  Profiling Averages:");

    flushAllprintsAbove();

    float mainLoopAvgMs = 0.0f;
    float totalLoadMs = 0.0f;

    for (int i = 0; i < PERF_LABEL_COUNT; ++i) {
        const auto& a = perfTable[i];
        float avgMs = a.cnt ? (a.sumUs / (float)a.cnt) / 1000.0f : 0.0f;

        // We dont show it if its 0
        if (avgMs < 0.01f) continue;  // Skip if average is zero

        appendOnly_perfDebugPrintf("    ∘ %-15s : %6.2f ms\n", perfLabelNames[i], avgMs);
        if (perfIncludedInLoad[i]) totalLoadMs += avgMs;
        if (i == PERF_MAIN_LOOP) mainLoopAvgMs = avgMs;
    }

    for (int i = 0; i < PERF_LABEL_COUNT; ++i) {
        perfTable[i].sumUs = 0;
        perfTable[i].cnt = 0;
    }

    flushAllprintsAbove();

    appendOnly_perfDebugPrintln("+----------------------------------------------------------------+");
    appendOnly_perfDebugPrintln("🕑  System Status:");

    constexpr float frameMs = 1000.0f / POLLING_RATE_HZ;

    float pollLoadPct = (totalLoadMs / frameMs) * 100.0f;
    float headroomMs  = frameMs - totalLoadMs;
    float headroomPct = 100.0f - pollLoadPct;
    float scaleFactor = totalLoadMs > 0.0f ? (frameMs / totalLoadMs) : 0.0f;
    uint64_t uptimeSec = esp_timer_get_time() / 1000000ULL;
    uint32_t mins = uptimeSec / 60;
    uint32_t secs = uptimeSec % 60;
    int cpuMHz = ESP.getCpuFreqMHz();
    const char* rr = _resetReasonToString(esp_reset_reason());

    appendOnly_perfDebugPrintf("    ∘ Poll Load     : %5.1f%% of %.2f ms slot\n", pollLoadPct, frameMs);
    appendOnly_perfDebugPrintf("    ∘ Headroom      : %5.3f ms (%5.1f%%)\n", headroomMs, headroomPct);
    appendOnly_perfDebugPrintf("    ∘ Scale Cap.    : %5.2fx current workload\n", scaleFactor);
    if (mins)
        appendOnly_perfDebugPrintf("    ∘ Uptime        : %lum%02lus\n", mins, secs);
    else
        appendOnly_perfDebugPrintf("    ∘ Uptime        : %4lus\n", secs);
    appendOnly_perfDebugPrintf("    ∘ CPU Frequency : %3d MHz\n", cpuMHz);
    appendOnly_perfDebugPrintf("    ∘ Last Reset    : %s\n", rr);

    appendOnly_perfDebugPrintln("+----------------------------------------------------------------+");

    flushAllprintsAbove();

    size_t free_int    = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t largest_int = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    float  frag_int    = free_int
                       ? 100.0f * (1.0f - (float)largest_int / (float)free_int)
                       : 0.0f;
    size_t free_psram    = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t largest_psram = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    float  frag_psram    = free_psram
                         ? 100.0f * (1.0f - (float)largest_psram / (float)free_psram)
                         : 0.0f;

    appendOnly_perfDebugPrintln("💾  Memory Fragmentation:");
    appendOnly_perfDebugPrintf("    ∘ Internal SRAM : free %6u KB, largest %6u KB, frag %5.1f%%\n",
                    (unsigned)(free_int/1024), (unsigned)(largest_int/1024), frag_int);
    appendOnly_perfDebugPrintf("    ∘ PSRAM Pool    : free %6u KB, largest %6u KB, frag %5.1f%%\n",
                    (unsigned)(free_psram/1024), (unsigned)(largest_psram/1024), frag_psram);

    appendOnly_perfDebugPrintln("+----------------------------------------------------------------+");

    #if (ARDUINO_USB_CDC_ON_BOOT == 1)
    int rxWaiting = Serial.available();
    int txAvail   = tud_cdc_write_available();  // NOT Serial.availableForWrite()
    #else
    int rxWaiting = 0;
    int txAvail   = 0;    
    #endif

    flushAllprintsAbove();

    appendOnly_perfDebugPrintln("📡  USB-CDC Buffer Health:");
    appendOnly_perfDebugPrintf("    ∘ TX Free Slots : %6d bytes\n", txAvail);
    appendOnly_perfDebugPrintf("    ∘ RX Pending    : %6d bytes\n", rxWaiting);
    appendOnly_perfDebugPrintln("+----------------------------------------------------------------+");
    appendOnly_perfDebugPrintln("🔁  Ring Buffer Health:");

    appendOnly_perfDebugPrintf("    ∘ Serial Debug     : avg %5.1f bytes, max %u bytes, pending %2u, max %2u, overruns %u\n",
    serialDebugAvgMsgLen(), serialDebugMaxMsgLen(),
    serialDebugGetPending(), serialDebugGetHighWater(), serialDebugGetOverflow());

#if (USE_DCSBIOS_WIFI || DEBUG_USE_WIFI)
    // UDP Send debug ring buffer health
    appendOnly_perfDebugPrintf("    ∘ UDP Send Debug   : avg %5.1f bytes, max %u bytes, pending %2u, max %2u, overruns %u\n",
    wifiDebugSendAvgMsgLen(), wifiDebugSendMaxMsgLen(),
    wifiDebugSendGetPending(), wifiDebugSendGetHighWater(), wifiDebugSendGetOverflow());
#endif

#if (USE_DCSBIOS_WIFI || USE_DCSBIOS_USB)
    // UDP Receive buffer health
    appendOnly_perfDebugPrintf("    ∘ UDP Receive DCS  : avg %5.1f bytes, max %u bytes, pending %2u, max %2u, overruns %u\n",
    dcsUdpRecvAvgMsgLen(), dcsUdpRecvMaxMsgLen(),
    dcsUdpRecvGetPending(), dcsUdpRecvGetHighWater(), dcsUdpRecvGetOverflow());    

    // USB Send buffer health
    appendOnly_perfDebugPrintf("    ∘ USB Send DCS  : avg %5.1f bytes, max %u bytes, pending %2u, max %2u, overruns %u\n",
    dcsRawUsbOutAvgMsgLen(), dcsRawUsbOutMaxMsgLen(),
    dcsRawUsbOutGetPending(), dcsRawUsbOutGetHighWater(), dcsRawUsbOutGetOverflow());
#endif

    appendOnly_perfDebugPrintln("+----------------------------------------------------------------+");

    // In one go, flush all appended entries from the buffer.
    flushAllprintsAbove();

    #if DEBUG_PERFORMANCE_SHOW_TASKS
    printTaskList();
    #endif
}
#endif // DEBUG_PERFORMANCE