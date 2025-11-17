// LEDControl.cpp

#include "../Globals.h"
#include "../LEDControl.h"
#include "../DCSBIOSBridge.h"

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

            AnalogG_set(led->info.gaugeInfo.gpio, rawValue); // this uses a task to update the servo every 20 ms (50Hz)
            
            #if DEBUG_PERFORMANCE
            endProfiling(PERF_LED_GAUGE);
            #endif            
            break;

        case DEVICE_PCA9555:
            #if DEBUG_PERFORMANCE
            beginProfiling(PERF_LED_PCA9555);
            #endif        
            PCA9555_write(
                led->info.pcaInfo.address,
                led->info.pcaInfo.port,
                led->info.pcaInfo.bit,
                led->activeLow ? !state : state
            );
            #if DEBUG_PERFORMANCE
            endProfiling(PERF_LED_PCA9555);
            #endif            
            break;

            /*
        case DEVICE_TM1637: {
            #if DEBUG_PERFORMANCE
            beginProfiling(PERF_LED_TM1637);
            #endif            
            auto& device = (led->info.tm1637Info.dioPin == RA_DIO_PIN) ? RA_Device : LA_Device;
            tm1637_displaySingleLED(device,
                led->info.tm1637Info.segment,
                led->info.tm1637Info.bit,
                state
            );
            #if DEBUG_PERFORMANCE
            endProfiling(PERF_LED_TM1637);
            #endif            
            break;
        }
        */

        case DEVICE_TM1637: {
            #if DEBUG_PERFORMANCE
            beginProfiling(PERF_LED_TM1637);
            #endif            

            if (led->info.tm1637Info.dioPin == RA_DIO_PIN) {
                tm1637_displaySingleLED(RA_Device, led->info.tm1637Info.segment, led->info.tm1637Info.bit, state);
            }
            else if (led->info.tm1637Info.dioPin == LA_DIO_PIN) {
                tm1637_displaySingleLED(LA_Device, led->info.tm1637Info.segment, led->info.tm1637Info.bit, state);
            }
            else if (led->info.tm1637Info.dioPin == JETT_DIO_PIN) {
                tm1637_displaySingleLED(JETSEL_Device, led->info.tm1637Info.segment, led->info.tm1637Info.bit, state);
            }
            else {
                debugPrintf("TM1637 unknown DIO pin: %u\n", led->info.tm1637Info.dioPin);
            }

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

        case DEVICE_NONE:
        default:
            #if DEBUG_PERFORMANCE
            beginProfiling(PERF_LED_UNKNOWN);
            #endif        
            if(DEBUG) debugPrintf("⚠️ '%s' is NOT a LED or has not being configured yet\n", label);
            #if DEBUG_PERFORMANCE
            endProfiling(PERF_LED_UNKNOWN);
            #endif            
            break;
    }
}