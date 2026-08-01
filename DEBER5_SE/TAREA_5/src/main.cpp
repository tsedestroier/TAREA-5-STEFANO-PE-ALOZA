#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

// UART2 configuration
#define UART_PORT_NUM      UART_NUM_2
#define UART_BAUD_RATE     115200
#define UART_TX_PIN        GPIO_NUM_17
#define UART_RX_PIN        GPIO_NUM_16
#define UART_BUFFER_SIZE   1024

// LED pins
#define LED_GREEN_PIN      GPIO_NUM_32
#define LED_RED_PIN        GPIO_NUM_33

// Command buffer size
#define CMD_BUF_SIZE       1024

static const char *TAG = "UART_EX1";
static int command_count = 0;

// ---------------------------------------------------------------------------
// GPIO (LED) initialization
// ---------------------------------------------------------------------------
static void init_gpio(void) {
    // Green LED
    gpio_reset_pin(LED_GREEN_PIN);
    gpio_set_direction(LED_GREEN_PIN, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(LED_GREEN_PIN, 0);

    // Red LED
    gpio_reset_pin(LED_RED_PIN);
    gpio_set_direction(LED_RED_PIN, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(LED_RED_PIN, 0);
}

// ---------------------------------------------------------------------------
// UART2 initialization (ESP-IDF native driver)
// ---------------------------------------------------------------------------
static void init_uart(void) {
    uart_config_t cfg = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_BUFFER_SIZE * 2,
                                         UART_BUFFER_SIZE * 2, 0, NULL, 0));
    ESP_LOGI(TAG, "UART2 configurado (TX=%d, RX=%d)", UART_TX_PIN, UART_RX_PIN);
}

// ---------------------------------------------------------------------------
// Helper: trim and lower-case a command string
// ---------------------------------------------------------------------------
static void clean_command(char *dst, const char *src) {
    int idx = 0;
    for (int i = 0; src[i] && idx < CMD_BUF_SIZE - 1; ++i) {
        if (src[i] == ' ' || src[i] == '\t') {
            if (idx > 0 && dst[idx-1] != ' ') {
                dst[idx++] = ' ';
            }
        } else {
            dst[idx++] = tolower((unsigned char)src[i]);
        }
    }
    if (idx > 0 && dst[idx-1] == ' ') idx--;
    dst[idx] = '\0';
}

// ---------------------------------------------------------------------------
// UART receive task – non-blocking, parses commands
// ---------------------------------------------------------------------------
static void uart_rx_task(void *arg) {
    uint8_t data[UART_BUFFER_SIZE];
    char cmd_buf[CMD_BUF_SIZE] = {0};
    int cmd_len = 0;

    const char *welcome = "\r\n=======================================================\r\n"
                          "   TAREA 5 - EJERCICIO 1: COMUNICACION UART2 (ESP-IDF) \r\n"
                          "=======================================================\r\n"
                          "[UART2 Ready] Ingrese comandos (status, info, led on, led off, led red on, led red off, reset)\r\n"
                          "> ";
    uart_write_bytes(UART_PORT_NUM, welcome, strlen(welcome));

    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, data, UART_BUFFER_SIZE - 1, pdMS_TO_TICKS(20));
        if (len > 0) {
            for (int i = 0; i < len; ++i) {
                char c = (char)data[i];
                if (c == '\b' || c == 127) {
                    if (cmd_len > 0) {
                        cmd_len--;
                        const char *bs = "\b \b";
                        uart_write_bytes(UART_PORT_NUM, bs, 3);
                    }
                } else if (c == '\r' || c == '\n') {
                    uart_write_bytes(UART_PORT_NUM, "\r\n", 2);
                    if (cmd_len > 0) {
                        cmd_buf[cmd_len] = '\0';
                        command_count++;
                        char clean[CMD_BUF_SIZE];
                        clean_command(clean, cmd_buf);

                        if (strcmp(clean, "status") == 0) {
                            int g = gpio_get_level(LED_GREEN_PIN);
                            int r = gpio_get_level(LED_RED_PIN);
                            uint64_t uptime = esp_timer_get_time() / 1000000ULL;
                            char resp[256];
                            snprintf(resp, sizeof(resp),
                                     "\r\n>>> STATUS:\r\n"
                                     "  LED Verde (GPIO32): %s\r\n"
                                     "  LED Rojo (GPIO33):  %s\r\n"
                                     "  Uptime (s):        %llu\r\n"
                                     "  Total comandos:    %d\r\n",
                                     g ? "ENCENDIDO" : "APAGADO",
                                     r ? "ENCENDIDO" : "APAGADO",
                                     uptime,
                                     command_count);
                            uart_write_bytes(UART_PORT_NUM, resp, strlen(resp));
                        } else if (strcmp(clean, "info") == 0) {
                            char resp[256];
                            snprintf(resp, sizeof(resp),
                                     "\r\n>>> INFO:\r\n"
                                     "  Puerto:        UART2 (GPIO17/TX, GPIO16/RX)\r\n"
                                     "  Baudrate:      %d\r\n"
                                     "  Arquitectura:  ESP32 Dual-Core\r\n"
                                     "  Comandos procesados: %d\r\n",
                                     UART_BAUD_RATE, command_count);
                            uart_write_bytes(UART_PORT_NUM, resp, strlen(resp));
                        } else if (strcmp(clean, "led on") == 0 || strcmp(clean, "led green on") == 0) {
                            gpio_set_level(LED_GREEN_PIN, 1);
                            const char *msg = ">>> LED Verde ENCENDIDO\r\n";
                            uart_write_bytes(UART_PORT_NUM, msg, strlen(msg));
                        } else if (strcmp(clean, "led off") == 0 || strcmp(clean, "led green off") == 0) {
                            gpio_set_level(LED_GREEN_PIN, 0);
                            const char *msg = ">>> LED Verde APAGADO\r\n";
                            uart_write_bytes(UART_PORT_NUM, msg, strlen(msg));
                        } else if (strcmp(clean, "led red on") == 0) {
                            gpio_set_level(LED_RED_PIN, 1);
                            const char *msg = ">>> LED Rojo ENCENDIDO\r\n";
                            uart_write_bytes(UART_PORT_NUM, msg, strlen(msg));
                        } else if (strcmp(clean, "led red off") == 0) {
                            gpio_set_level(LED_RED_PIN, 0);
                            const char *msg = ">>> LED Rojo APAGADO\r\n";
                            uart_write_bytes(UART_PORT_NUM, msg, strlen(msg));
                        } else if (strcmp(clean, "reset") == 0) {
                            command_count = 0;
                            const char *msg = ">>> Contador reiniciado a 0\r\n";
                            uart_write_bytes(UART_PORT_NUM, msg, strlen(msg));
                        } else {
                            char resp[256];
                            snprintf(resp, sizeof(resp),
                                     "\r\n>>> ERROR: comando no reconocido '%s'\r\n"
                                     "  Comandos validos: status, info, reset, led on, led off, led red on, led red off\r\n",
                                     clean);
                            uart_write_bytes(UART_PORT_NUM, resp, strlen(resp));
                        }
                        cmd_len = 0;
                    }
                    uart_write_bytes(UART_PORT_NUM, "> ", 2);
                } else {
                    if (cmd_len < CMD_BUF_SIZE - 1) {
                        cmd_buf[cmd_len++] = c;
                        uart_write_bytes(UART_PORT_NUM, &c, 1);
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    uart_driver_delete(UART_PORT_NUM);
    vTaskDelete(NULL);
}

// ---------------------------------------------------------------------------
// Application entry point (ESP-IDF)
// --------------------------------------------------------------------------
void app_main(void) {
    init_gpio();
    init_uart();
    xTaskCreatePinnedToCore(uart_rx_task, "uart_rx_task", 4096, NULL, 5, NULL, 1);
}