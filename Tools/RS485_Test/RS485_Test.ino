/*
 * ESP32 Simple RS485 Test Master
 * Polls slave and prints received input commands to Serial
 */

#include <driver/uart.h>

#define RS485_UART      UART_NUM_1
#define RS485_TX        17
#define RS485_RX        18
#define RS485_EN        21
#define RS485_BAUD      250000
#define SLAVE_ADDR      1

uint32_t polls = 0, dataResps = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    uart_config_t cfg = {
        .baud_rate = RS485_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT
    };
    uart_driver_install(RS485_UART, 256, 256, 0, NULL, 0);
    uart_param_config(RS485_UART, &cfg);
    uart_set_pin(RS485_UART, RS485_TX, RS485_RX, RS485_EN, UART_PIN_NO_CHANGE);
    uart_set_mode(RS485_UART, UART_MODE_RS485_HALF_DUPLEX);
    
    Serial.println("Test Master Ready");
}

void loop() {
    polls++;

    // Send empty poll: [Addr][MsgType=0][Len=0]
    uint8_t poll[] = { SLAVE_ADDR, 0, 0 };
    uart_write_bytes(RS485_UART, poll, 3);
    uart_wait_tx_done(RS485_UART, pdMS_TO_TICKS(10));
    
    delay(5);
    
    // Simulate CockpitOS: read byte-by-byte with delays between
    uint8_t buf[128];
    int n = 0;
    
    for (int i = 0; i < 50; i++) {  // Max 50 iterations
        size_t avail = 0;
        uart_get_buffered_data_len(RS485_UART, &avail);
        
        if (avail > 0) {
            uart_read_bytes(RS485_UART, &buf[n], 1, 0);
            n++;
            if (n >= 128) break;
        } else if (n > 0) {
            // Had data, now empty - packet done
            break;
        }
        
        delayMicroseconds(200);  // Simulate other CockpitOS work
    }
    
    if (n > 1) {
        dataResps++;
        Serial.print("[INPUT] ");
        for (int i = 2; i < n - 1; i++) {
            Serial.print((char)buf[i]);
        }
        Serial.println();
    }
    
    // Stats every 10 sec
    static uint32_t lastStat = 0;
    if (millis() - lastStat > 10000) {
        lastStat = millis();
        Serial.printf("--- Polls=%u Commands=%u ---\n", polls, dataResps);
    }
    
    delay(100);
}