#pragma once

// PSRAM detection & malloc/free redirection header
// Include this before any other library that uses malloc/free (e.g. WiFi.h, WiFiUDP.h)

#include "sdkconfig.h"
#include <esp_heap_caps.h>
#include "esp32-hal-psram.h"
#include <stdlib.h>
#include <utility>

/**
 * @brief If built with PSRAM support, redirect all heap allocations
 *        into external PSRAM by default.
 *        Must be included *before* any includes of WiFi or lwIP headers.
 */
#ifdef CONFIG_SPIRAM_SUPPORT
  // Redirect standard malloc/free to PSRAM
  #undef malloc
  #undef free
  #define malloc(sz)   heap_caps_malloc((sz), MALLOC_CAP_SPIRAM)
  #define free(ptr)    heap_caps_free(ptr)
#endif

/**
 * @brief Initialize and detect external PSRAM at runtime.
 * @return true if PSRAM is initialized and usable, false otherwise.
 */
static inline bool initPSRAM() {
#ifdef CONFIG_SPIRAM_SUPPORT
    psramInit();  // initialize driver
#endif
    // return true if PSRAM heap has non-zero capacity
    return heap_caps_get_total_size(MALLOC_CAP_SPIRAM) > 0;
}

/**
 * @brief Allocate memory preferentially from PSRAM if available.
 *        Falls back to internal heap otherwise.
 */
#ifdef CONFIG_SPIRAM_SUPPORT
  #define PS_MALLOC(bytes)    heap_caps_malloc((bytes), MALLOC_CAP_SPIRAM)
  #define PS_FREE(ptr)        heap_caps_free(ptr)
  #define PS_ATTR             __attribute__((section(".psram")))
#else
  #define PS_MALLOC(bytes)    malloc(bytes)
  #define PS_FREE(ptr)        free(ptr)
  #define PS_ATTR
#endif

/*
template <typename T>
T* ps_new() {
    void* mem = PS_MALLOC(sizeof(T));
    return mem ? new (mem) T : nullptr;
}
*/

template <typename T, typename... Args>
T* ps_new(Args&&... args) {
    void* mem = PS_MALLOC(sizeof(T));
    return mem ? new (mem) T(std::forward<Args>(args)...) : nullptr;
}