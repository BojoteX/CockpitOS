/**
 * RS-485 Transceiver Reset & Diagnostic Sketch
 * For Waveshare ESP32-S3-RS485-CAN board
 * 
 * PURPOSE: Force an AUTO-DIRECTION RS-485 transceiver out of a stuck state.
 * 
 * Auto-direction transceivers (like MAX13487, SP3485 with auto-enable, etc.)
 * detect TX activity to switch between TX/RX modes automatically.
 * They can get stuck in TX mode if:
 *   - TX line is held LOW (break condition)
 *   - UART peripheral left in bad state
 *   - Glitch during boot
 * 
 * Solution: Force TX HIGH (idle) for sufficient time to let the transceiver
 * timeout and return to RX mode.
 */

#include <driver/uart.h>
#include <driver/gpio.h>

// Pin definitions - Waveshare ESP32-S3-RS485-CAN (AUTO-DIRECTION)
#define RS485_TX_PIN    17      // UART TX -> RS485 DI
#define RS485_RX_PIN    18      // UART RX <- RS485 RO  
// NO ENABLE PIN - transceiver has auto-direction

#define RS485_UART_NUM  UART_NUM_1
#define RS485_BAUD      250000

void forceGpioReset() {
    Serial.println("\n=== PHASE 1: Force GPIO Reset (Auto-Direction Transceiver) ===");
    
    // First, ensure any existing UART driver is removed
    Serial.println("Deleting any existing UART driver...");
    uart_driver_delete(RS485_UART_NUM);
    delay(10);
    
    // CRITICAL: Disconnect TX from UART peripheral and force HIGH
    // Auto-direction transceivers switch to RX mode when TX is idle (HIGH)
    Serial.println("Forcing TX pin HIGH to trigger auto-direction timeout...");
    
    gpio_reset_pin((gpio_num_t)RS485_TX_PIN);
    gpio_set_direction((gpio_num_t)RS485_TX_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)RS485_TX_PIN, 1);  // IDLE HIGH - critical!
    Serial.println("  TX (GPIO17) -> OUTPUT, HIGH (idle state)");
    
    // RX pin - set as input with pullup
    gpio_reset_pin((gpio_num_t)RS485_RX_PIN);
    gpio_set_direction((gpio_num_t)RS485_RX_PIN, GPIO_MODE_INPUT);
    gpio_pullup_en((gpio_num_t)RS485_RX_PIN);
    Serial.println("  RX (GPIO18) -> INPUT with pullup");
    
    // Hold TX HIGH for 500ms - this is way longer than any auto-direction
    // timeout (typically 500ns to 50µs depending on chip)
    Serial.println("Holding TX HIGH for 500ms to reset auto-direction...");
    delay(500);
    
    Serial.println("GPIO reset complete. Transceiver should be in RX mode now.");
}

void toggleTxLine() {
    Serial.println("\n=== PHASE 2: TX Line Manipulation ===");
    Serial.println("Sending idle pattern to exercise auto-direction...");
    
    // Send some HIGH-LOW-HIGH transitions to "wake up" the transceiver
    // while ensuring we end in IDLE (HIGH) state
    for (int i = 0; i < 10; i++) {
        gpio_set_level((gpio_num_t)RS485_TX_PIN, 0);
        delayMicroseconds(100);  // Brief LOW
        gpio_set_level((gpio_num_t)RS485_TX_PIN, 1);
        delayMicroseconds(500);  // Longer HIGH (idle)
    }
    
    // Final long idle
    gpio_set_level((gpio_num_t)RS485_TX_PIN, 1);
    delay(100);
    
    Serial.println("TX line exercise complete. Left in IDLE (HIGH) state.");
}

void readPinStates() {
    Serial.println("\n=== Current Pin States ===");
    Serial.printf("  TX (GPIO%d): %d  [should be 1/HIGH for idle]\n", 
                  RS485_TX_PIN, gpio_get_level((gpio_num_t)RS485_TX_PIN));
    Serial.printf("  RX (GPIO%d): %d  [depends on bus state]\n", 
                  RS485_RX_PIN, gpio_get_level((gpio_num_t)RS485_RX_PIN));
}

bool initUart() {
    Serial.println("\n=== PHASE 3: UART Initialization ===");
    
    uart_config_t uart_config = {
        .baud_rate = RS485_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    // Install driver with buffer
    esp_err_t err = uart_driver_install(RS485_UART_NUM, 256, 256, 0, NULL, 0);
    if (err != ESP_OK) {
        Serial.printf("ERROR: uart_driver_install failed: %s\n", esp_err_to_name(err));
        return false;
    }
    Serial.println("UART driver installed.");
    
    err = uart_param_config(RS485_UART_NUM, &uart_config);
    if (err != ESP_OK) {
        Serial.printf("ERROR: uart_param_config failed: %s\n", esp_err_to_name(err));
        return false;
    }
    Serial.println("UART params configured.");
    
    // Set pins - NO RTS/CTS (auto-direction handles it)
    err = uart_set_pin(RS485_UART_NUM, RS485_TX_PIN, RS485_RX_PIN, 
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        Serial.printf("ERROR: uart_set_pin failed: %s\n", esp_err_to_name(err));
        return false;
    }
    Serial.println("UART pins assigned (no RTS - auto-direction transceiver).");
    
    // Use standard UART mode - transceiver handles direction automatically
    // Do NOT use UART_MODE_RS485_HALF_DUPLEX since there's no RTS pin to control
    err = uart_set_mode(RS485_UART_NUM, UART_MODE_UART);
    if (err != ESP_OK) {
        Serial.printf("ERROR: uart_set_mode failed: %s\n", esp_err_to_name(err));
        return false;
    }
    Serial.println("UART mode set to standard (transceiver auto-direction).");
    
    // Flush any garbage
    uart_flush(RS485_UART_NUM);
    Serial.println("UART buffers flushed.");
    
    return true;
}

void testTransmit() {
    Serial.println("\n=== TEST: Transmit ===");
    
    uint8_t testData[] = {0x55, 0x55, 0x55, 0x55, 0xAA};  // Sync pattern + marker
    
    Serial.println("Sending test pattern: 55 55 55 55 AA");
    int written = uart_write_bytes(RS485_UART_NUM, testData, sizeof(testData));
    esp_err_t err = uart_wait_tx_done(RS485_UART_NUM, pdMS_TO_TICKS(100));
    
    Serial.printf("Wrote %d bytes, wait_tx_done: %s\n", written, esp_err_to_name(err));
    
    // With auto-direction, we should see echo if RX is connected to bus
    delay(10);
    uint8_t echo[16];
    int echoLen = uart_read_bytes(RS485_UART_NUM, echo, sizeof(echo), pdMS_TO_TICKS(50));
    if (echoLen > 0) {
        Serial.printf("Echo received (%d bytes): ", echoLen);
        for (int i = 0; i < echoLen; i++) Serial.printf("%02X ", echo[i]);
        Serial.println();
    }
}

void testReceive() {
    Serial.println("\n=== TEST: Receive (5 second window) ===");
    Serial.println("Send data to the RS-485 bus now...");
    
    uint8_t buf[64];
    unsigned long start = millis();
    int totalReceived = 0;
    
    while (millis() - start < 5000) {
        int len = uart_read_bytes(RS485_UART_NUM, buf, sizeof(buf), pdMS_TO_TICKS(100));
        if (len > 0) {
            Serial.printf("Received %d bytes: ", len);
            for (int i = 0; i < len; i++) {
                Serial.printf("%02X ", buf[i]);
            }
            Serial.println();
            totalReceived += len;
        }
    }
    
    Serial.printf("Receive test complete. Total: %d bytes\n", totalReceived);
}

void sendBreakCondition() {
    Serial.println("\n=== Sending BREAK condition (diagnostic) ===");
    Serial.println("This forces TX LOW which may re-trigger stuck state!");
    
    // Delete UART, take manual control
    uart_driver_delete(RS485_UART_NUM);
    delay(10);
    
    gpio_reset_pin((gpio_num_t)RS485_TX_PIN);
    gpio_set_direction((gpio_num_t)RS485_TX_PIN, GPIO_MODE_OUTPUT);
    
    // Send break (TX LOW for extended time)
    gpio_set_level((gpio_num_t)RS485_TX_PIN, 0);
    Serial.println("TX LOW (break) for 100ms...");
    delay(100);
    
    // Return to idle
    gpio_set_level((gpio_num_t)RS485_TX_PIN, 1);
    Serial.println("TX HIGH (idle) for 500ms...");
    delay(500);
    
    // Reinit UART
    initUart();
    Serial.println("BREAK test complete.");
}

void setup() {
    Serial.begin(115200);
    delay(2000);  // Wait for serial monitor
    
    Serial.println("\n");
    Serial.println("================================================");
    Serial.println("   RS-485 TRANSCEIVER RESET UTILITY");
    Serial.println("   For AUTO-DIRECTION Transceiver");
    Serial.println("   (No enable pin - hardware auto-switches)");
    Serial.println("================================================");
    Serial.printf("TX: GPIO%d, RX: GPIO%d\n", RS485_TX_PIN, RS485_RX_PIN);
    Serial.printf("Baud: %d\n", RS485_BAUD);
    
    // Phase 1: Force GPIO reset - most important for auto-direction!
    forceGpioReset();
    readPinStates();
    
    // Phase 2: Exercise TX line
    toggleTxLine();
    readPinStates();
    
    // Phase 3: Initialize UART
    if (!initUart()) {
        Serial.println("\nFATAL: UART init failed!");
        while(1) delay(1000);
    }
    
    // Quick tests
    testTransmit();
    testReceive();
    
    Serial.println("\n================================================");
    Serial.println("   RESET COMPLETE");
    Serial.println("================================================");
    Serial.println("\nCommands:");
    Serial.println("  't' - Transmit test");
    Serial.println("  'r' - Receive test (5 sec)");
    Serial.println("  'p' - Print pin states");
    Serial.println("  'x' - Full reset sequence");
    Serial.println("  'b' - Send BREAK (may re-trigger stuck state!)");
}

void loop() {
    if (Serial.available()) {
        char cmd = Serial.read();
        
        switch (cmd) {
            case 't':
            case 'T':
                testTransmit();
                break;
            case 'r':
            case 'R':
                testReceive();
                break;
            case 'p':
            case 'P':
                readPinStates();
                break;
            case 'x':
            case 'X':
                Serial.println("\n*** FULL RESET ***");
                forceGpioReset();
                toggleTxLine();
                initUart();
                Serial.println("Reset complete.");
                break;
            case 'b':
            case 'B':
                sendBreakCondition();
                break;
        }
    }
    
    // Show any incoming RS-485 data
    uint8_t buf[64];
    int len = uart_read_bytes(RS485_UART_NUM, buf, sizeof(buf), 0);
    if (len > 0) {
        Serial.printf("[RX %d]: ", len);
        for (int i = 0; i < len; i++) {
            Serial.printf("%02X ", buf[i]);
        }
        Serial.println();
    }
}
