// LEDControl.cpp

#include "../Globals.h"
#include "../LEDControl.h"
#include "../DCSBIOSBridge.h"

// ============================================================================
// OUTPUT DRIVER PRESENCE FLAGS
// Set by scanOutputDevicePresence(), used by tickOutputDrivers()
// ============================================================================
static bool g_hasTM1637 = false;
static bool g_hasWS2812 = false;
static bool g_hasGN1640 = false;
static bool g_hasGauge = false;
static bool g_hasMagnetic = false;
static bool g_deviceScanComplete = false;

// ============================================================================
// scanOutputDevicePresence() - Scan LEDMapping to detect which drivers needed
// Called once during initializeLEDs()
// ============================================================================
void scanOutputDevicePresence() {
    g_hasTM1637 = false;
    g_hasWS2812 = false;
    g_hasGN1640 = false;
    g_hasGauge = false;
    g_hasMagnetic = false;

    for (uint16_t i = 0; i < panelLEDsCount; ++i) {
        switch (panelLEDs[i].deviceType) {
        case DEVICE_TM1637:   g_hasTM1637 = true;   break;
        case DEVICE_WS2812:   g_hasWS2812 = true;   break;
        case DEVICE_GN1640T:  g_hasGN1640 = true;   break;
        case DEVICE_GAUGE:    g_hasGauge = true;     break;
        case DEVICE_MAGNETIC: g_hasMagnetic = true;  break;
        default: break;
        }
    }

    g_deviceScanComplete = true;

    debugPrintf("📊 Output device scan: TM1637=%d WS2812=%d GN1640=%d GAUGE=%d MAGNETIC=%d\n",
        g_hasTM1637, g_hasWS2812, g_hasGN1640, g_hasGauge, g_hasMagnetic);
}

// ============================================================================
// hasOutputDevice() - Query if a device type exists in LEDMapping
// ============================================================================
bool hasOutputDevice(uint8_t deviceType) {
    if (!g_deviceScanComplete) {
        debugPrintln("⚠️ hasOutputDevice() called before scanOutputDevicePresence()!");
        return false;
    }
    switch (deviceType) {
    case DEVICE_TM1637:   return g_hasTM1637;
    case DEVICE_WS2812:   return g_hasWS2812;
    case DEVICE_GN1640T:  return g_hasGN1640;
    case DEVICE_GAUGE:    return g_hasGauge;
    case DEVICE_MAGNETIC: return g_hasMagnetic;
    default: return false;
    }
}

// ============================================================================
// tickOutputDrivers() - Flush all output driver buffers
// Called each frame from panelLoop(), idempotent (safe to call multiple times)
// ============================================================================
void tickOutputDrivers() {
    if (g_hasTM1637)  tm1637_tick();
    if (g_hasWS2812)  WS2812_tick();
    if (g_hasGN1640)  GN1640_tick();
    if (g_hasGauge)   AnalogG_tick();   // service servo gauges
}

// Blazing fast setLED()
void setLED(const char* label, bool state, uint8_t intensity, uint16_t rawValue, uint16_t maxValue) {

    const LEDMapping* led = findLED(label);
    if (!led) {
        if(DEBUG) debugPrintf("⚠️ LED label '%s' not found\n", label);
        return;
    }

    switch (led->deviceType) {

        case DEVICE_GPIO:
    #if DEBUG_PERFORMANCE
            beginProfiling(PERF_LED_GPIO);
    #endif

            if (led->dimmable) {
                // Map intensity (0-100) to (0-255), then invert if activeLow
                uint8_t pwmValue = map(intensity, 0, 100, 0, 255);
                if (led->activeLow) pwmValue = 255 - pwmValue;
                analogWrite(led->info.gpioInfo.gpio, state ? pwmValue : (led->activeLow ? 255 : 0));
                // If state==false: 0 for activeHigh, 255 for activeLow (full off)
            }
            else {
                // For digital, invert if activeLow
                uint8_t pinLevel = (state ^ led->activeLow) ? HIGH : LOW;
                digitalWrite(led->info.gpioInfo.gpio, pinLevel);
            }

    #if DEBUG_PERFORMANCE
            endProfiling(PERF_LED_GPIO);
    #endif            
            break;

        case DEVICE_GAUGE:
            #if DEBUG_PERFORMANCE
            beginProfiling(PERF_LED_GAUGE);
            #endif

            // DEVICE_GAUGE supports two modes:
            //   Analog (rawValue has full 0-65535 range): pass rawValue through
            //   Binary (called with just state, rawValue=0): use state to drive 0 or 65535
            {
                uint16_t gaugeValue = (maxValue >= 65535) ? rawValue : (state ? 65535 : 0);
                AnalogG_set(led->info.gaugeInfo.gpio, gaugeValue);
            }
            
            #if DEBUG_PERFORMANCE
            endProfiling(PERF_LED_GAUGE);
            #endif            
            break;

        case DEVICE_PCA9555:
            #if DEBUG_PERFORMANCE
            beginProfiling(PERF_LED_PCA9555);
            #endif   

            #if ENABLE_PCA9555
                PCA9555_write(
                    led->info.pcaInfo.address,
                    led->info.pcaInfo.port,
                    led->info.pcaInfo.bit,
                    led->activeLow ? !state : state
                );
            #endif

            #if DEBUG_PERFORMANCE
            endProfiling(PERF_LED_PCA9555);
            #endif            
            break;

        case DEVICE_TM1637: {
            #if DEBUG_PERFORMANCE
            beginProfiling(PERF_LED_TM1637);
            #endif            

            uint8_t clk = led->info.tm1637Info.clkPin;
            uint8_t dio = led->info.tm1637Info.dioPin;

            TM1637Device* dev = TM1637_findByPins(clk, dio);
            if (!dev) {
                debugPrintf("TM1637: no device for CLK=%u DIO=%u (LED %s)\n",
                    clk, dio, led->label);
                break;
            }

            tm1637_displaySingleLED(*dev,
                led->info.tm1637Info.segment,
                led->info.tm1637Info.bit,
                state);

            #if DEBUG_PERFORMANCE
            endProfiling(PERF_LED_TM1637);
            #endif            
            break;
        }

        case DEVICE_GN1640T:
            #if DEBUG_PERFORMANCE
            beginProfiling(PERF_LED_GN1640);
            #endif        
            GN1640_setLED(
                led->info.gn1640Info.column,
                led->info.gn1640Info.row,
                state
            );
            #if DEBUG_PERFORMANCE
            endProfiling(PERF_LED_GN1640);
            #endif            
            break;

        case DEVICE_WS2812:
            #if DEBUG_PERFORMANCE
            beginProfiling(PERF_LED_WS2812);
            #endif
            {
                const auto& w = led->info.ws2812Info;

                // Gain: use live intensity if dimmable, else the mapping’s default brightness
                const uint8_t gain = led->dimmable ? map(intensity, 0, 100, 0, 255) : w.defBright;

                // Fixed-point scale without floats
                auto scale8 = [&](uint8_t v) -> uint8_t {
                    return static_cast<uint8_t>((static_cast<uint16_t>(v) * gain) >> 8);
                    };

                // Color comes exclusively from mapping defaults, scaled by gain
                const CRGB color = state
                    ? CRGB(scale8(w.defR), scale8(w.defG), scale8(w.defB))
                    : Black;

                // Route to the correct strip by pin
                WS2812_setLEDColor(w.pin, w.index, color);
            }
            #if DEBUG_PERFORMANCE
            endProfiling(PERF_LED_WS2812);
            #endif
            break;

        case DEVICE_MAGNETIC:
            // Magnetic solenoid: single (2-pos) or dual (3-pos)
            {
                const auto& m = led->info.magneticInfo;
                uint8_t off = led->activeLow ? HIGH : LOW;
                uint8_t on  = led->activeLow ? LOW  : HIGH;

                if (m.gpioB == 255) {
                    // Single solenoid (2-pos): rest=0, energize when ON
                    digitalWrite(m.gpioA, (rawValue > 0) ? on : off);
                } else {
                    // Dual solenoid (3-pos): rest=1 (center)
                    if (rawValue < 1) {
                        digitalWrite(m.gpioA, on);
                        digitalWrite(m.gpioB, off);
                    } else if (rawValue > 1) {
                        digitalWrite(m.gpioA, off);
                        digitalWrite(m.gpioB, on);
                    } else {
                        digitalWrite(m.gpioA, off);
                        digitalWrite(m.gpioB, off);
                    }
                }
            }
            break;

        case DEVICE_NONE:
        default:
            #if DEBUG_PERFORMANCE
            beginProfiling(PERF_LED_UNKNOWN);
            #endif

            // if(DEBUG) debugPrintf("⚠️ '%s' is NOT a LED or has not being configured yet\n", label);

#if DEBUG_PERFORMANCE
            endProfiling(PERF_LED_UNKNOWN);
            #endif
            break;
    }

    // Device-type-aware debug output (single block, no extra lookup)
    if (DEBUG) {
        switch (led->deviceType) {
            case DEVICE_MAGNETIC: {
                const auto& m = led->info.magneticInfo;
                if (m.gpioB == 255) {
                    debugPrintf("[MAGNETIC] %s %s (raw=%u)\n", label,
                        rawValue > 0 ? "ENERGIZE" : "DE-ENERGIZE", rawValue);
                } else {
                    const char* pos = (rawValue < 1) ? "A" : (rawValue > 1) ? "B" : "REST";
                    debugPrintf("[MAGNETIC] %s %s (raw=%u)\n", label, pos, rawValue);
                }
                break;
            }
            case DEVICE_GAUGE:
                debugPrintf("[GAUGE] %s value=%u/%u\n", label, rawValue, maxValue);
                break;
            case DEVICE_NONE:
                break;
            default:
                if (led->dimmable && intensity < 100) {
                    debugPrintf("[LED] %s Intensity %u%%\n", label, intensity);
                } else {
                    debugPrintf("[LED] %s %s\n", label, state ? "ON" : "OFF");
                }
                break;
        }
    }
}