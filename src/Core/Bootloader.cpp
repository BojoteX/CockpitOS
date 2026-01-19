// Bootloader.cpp - Remote bootloader entry for firmware updates
// Extracted from RingBuffer.cpp for unconditional availability

#include "../Globals.h"
#include "esp_system.h"

// ═══════════════════════════════════════════════════════════════════════════
// REMOTE BOOTLOADER TRIGGER
// Called when magic packet "COCKPITOS:REBOOT:<target>\n" matches this device
// or when startup watchdog times out (recovery mode)
// ═══════════════════════════════════════════════════════════════════════════

// S2/S3 with TinyUSB have a ready-made function
#if (CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3) && (ARDUINO_USB_MODE == 0)
#include "esp32-hal-tinyusb.h"
#define USE_USB_PERSIST_RESTART 1
#else
#define USE_USB_PERSIST_RESTART 0
#endif

// Chip-specific register definitions
#if CONFIG_IDF_TARGET_ESP32S2
#include "soc/rtc_cntl_reg.h"
#define FORCE_DOWNLOAD_REG  RTC_CNTL_OPTION1_REG
#define FORCE_DOWNLOAD_BIT  RTC_CNTL_FORCE_DOWNLOAD_BOOT
#define BOOTLOADER_SUPPORTED 1
#elif CONFIG_IDF_TARGET_ESP32S3
#include "soc/rtc_cntl_reg.h"
#define FORCE_DOWNLOAD_REG  RTC_CNTL_OPTION1_REG
#define FORCE_DOWNLOAD_BIT  RTC_CNTL_FORCE_DOWNLOAD_BOOT
#define BOOTLOADER_SUPPORTED 1
#elif CONFIG_IDF_TARGET_ESP32C3
#include "soc/rtc_cntl_reg.h"
#define FORCE_DOWNLOAD_REG  RTC_CNTL_OPTION1_REG
#define FORCE_DOWNLOAD_BIT  RTC_CNTL_FORCE_DOWNLOAD_BOOT
#define BOOTLOADER_SUPPORTED 1
#elif CONFIG_IDF_TARGET_ESP32C6
#include "soc/lp_aon_reg.h"
#define FORCE_DOWNLOAD_REG  LP_AON_SYS_CFG_REG
#define FORCE_DOWNLOAD_BIT  LP_AON_FORCE_DOWNLOAD_BOOT
#define BOOTLOADER_SUPPORTED 1
#elif CONFIG_IDF_TARGET_ESP32H2
#include "soc/lp_aon_reg.h"
#define FORCE_DOWNLOAD_REG  LP_AON_SYS_CFG_REG
#define FORCE_DOWNLOAD_BIT  LP_AON_FORCE_DOWNLOAD_BOOT
#define BOOTLOADER_SUPPORTED 1
#else
// ESP32 Classic - hardware limitation, cannot enter bootloader programmatically
#define BOOTLOADER_SUPPORTED 0
#endif

#if BOOTLOADER_SUPPORTED && !USE_USB_PERSIST_RESTART
// Shutdown handler - executes at the RIGHT time during restart sequence
static void IRAM_ATTR bootloader_shutdown_handler(void) {
    REG_WRITE(FORCE_DOWNLOAD_REG, FORCE_DOWNLOAD_BIT);
}
#endif

void enterBootloaderMode() {

    debugPrintln("🔄 [BOOTLOADER] Entering firmware download mode...");
    delay(100);  // Let debug output flush

#if !BOOTLOADER_SUPPORTED
    // ESP32 Classic cannot do this
    debugPrintln("❌ [BOOTLOADER] ESP32 Classic cannot enter bootloader programmatically");
    debugPrintln("   Hardware limitation - use physical BOOT button or OTA updates");
    return;
#else

#if USE_USB_PERSIST_RESTART
    // S2/S3 with TinyUSB: Use the built-in API (handles USB peripheral properly)
    usb_persist_restart(RESTART_BOOTLOADER);
#else
    // All other supported chips: Use shutdown handler approach
    esp_err_t err = esp_register_shutdown_handler(bootloader_shutdown_handler);
    if (err == ESP_OK) {
        esp_restart();
    }
    else {
        debugPrintf("❌ [BOOTLOADER] Failed to register shutdown handler: %d\n", err);
    }
#endif

#endif

    // Should never reach here
    while (1) { delay(100); }
}
