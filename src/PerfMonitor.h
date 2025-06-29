// PerfMonitor.h

/*

    // Performance Profiling using beginProfiling("name") -> endProfiling("name") but only when DEBUG_PERFORMANCE  

    #if DEBUG_PERFORMANCE
    beginProfiling(PERF_DISPLAY_RENDER);
    #endif

	WHATEVER YOU WANT TO PROFILE

    #if DEBUG_PERFORMANCE
    endProfiling(PERF_DISPLAY_RENDER);
    #endif

*/

#pragma once

#if DEBUG_PERFORMANCE

const char* const perfLabelNames[] = {
    "[LED] GPIO",			// PERF_LED_GPIO
    "[LED] PCA9555",			// PERF_LED_PCA9555
    "[LED] TM1637",			// PERF_LED_TM1637
    "[LED] GN1640",			// PERF_LED_GN1640
    "[LED] WS2812",			// PERF_LED_WS2812
    "[LED] GAUGE",			// PERF_LED_GAUGE
    "[LED] Unknown Device",		// PERF_LED_UNKNOWN
    "[REPLAY] Replay Loop",		// PERF_REPLAY
    "[UDP] onDcsBiosUdpPacket",		// PERF_WIFI_DCSBIOS
    "HID SendReport",			// PERF_HIDREPORTS
    "DCSBIOS Listener",			// PERF_DCSBIOS
    "[DISPLAY] Renderer",		// PERF_DISPLAY_RENDER
    "[CUSTOM] Generic Profiler", 	// PERF_GENERIC_PROFILER
    "[CUSTOM] Generic Profiler 1", 	// PERF_GENERIC_PROFILER1
    "[CUSTOM] Generic Profiler 2", 	// PERF_GENERIC_PROFILER2
    "Main Loop" 			// PERF_MAIN_LOOP
};
enum PerfLabel : uint8_t {
    PERF_LED_GPIO,
    PERF_LED_PCA9555,
    PERF_LED_TM1637,
    PERF_LED_GN1640,
    PERF_LED_WS2812,
    PERF_LED_GAUGE,
    PERF_LED_UNKNOWN,
    PERF_REPLAY,
    PERF_WIFI_DCSBIOS,
    PERF_HIDREPORTS,
    PERF_DCSBIOS,
    PERF_DISPLAY_RENDER,
    PERF_GENERIC_PROFILER,
    PERF_GENERIC_PROFILER1,
    PERF_GENERIC_PROFILER2,
    PERF_MAIN_LOOP,
    PERF_LABEL_COUNT,  // 🔒 Sentinel for size enforcement
};
constexpr bool perfIncludedInLoad[PERF_LABEL_COUNT] = {
    false,  	// PERF_LED_GPIO
    false,  	// PERF_LED_PCA9555
    false,  	// PERF_LED_TM1637
    false,  	// PERF_LED_GN1640
    false,  	// PERF_LED_WS2812
    false,  	// PERF_LED_GAUGE
    false,  	// PERF_LED_UNKNOWN
    false,  	// PERF_REPLAY
    false,  	// PERF_WIFI_DCSBIOS
    false,  	// PERF_HIDREPORTS
    false,  	// PERF_DCSBIOS
    false,  	// PERF_DISPLAY_RENDER
    false, 	// PERF_GENERIC_PROFILER
    false, 	// PERF_GENERIC_PROFILER1
    false, 	// PERF_GENERIC_PROFILER2
    true   	// PERF_MAIN_LOOP
};
extern const char* const perfLabelNames[];

static_assert(PERF_LABEL_COUNT == sizeof(perfLabelNames) / sizeof(perfLabelNames[0]), "Mismatch: perfLabelNames[] size");
static_assert(PERF_LABEL_COUNT == sizeof(perfIncludedInLoad) / sizeof(perfIncludedInLoad[0]), "Mismatch: perfIncludedInLoad[] size");

struct ProfAccum {
    uint64_t sumUs;
    uint32_t cnt;
    uint32_t startUs;
};
extern ProfAccum perfTable[PERF_LABEL_COUNT];

void initPerfMonitor();
void beginProfiling(PerfLabel label);
void endProfiling(PerfLabel label);
void perfMonitorUpdate();
void printTaskList();

// static_assert(PERF_LABEL_COUNT == sizeof(perfLabelNames) / sizeof(perfLabelNames[0]), "perfLabelNames size mismatch");
// static_assert(PERF_LABEL_COUNT == sizeof(perfIncludedInLoad) / sizeof(perfIncludedInLoad[0]), "perfIncludedInLoad size mismatch");

#endif // DEBUG_PERFORMANCE