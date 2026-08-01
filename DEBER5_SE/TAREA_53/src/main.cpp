#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_timer.h"

// Incluimos los drivers nativos de la OLED provistos por la guía
#include "ssd1306.h"

// Definiciones de Hardware (Pines ESP32 nativos)
#define PIN_LED_GREEN   GPIO_NUM_32
#define PIN_LED_RED     GPIO_NUM_33
#define PIN_BTN         GPIO_NUM_13

// Configuración del bus I2C
#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_SDA_IO   21
#define I2C_MASTER_SCL_IO   22
#define I2C_MASTER_FREQ_HZ  400000

// Constantes de FreeRTOS y Tareas
#define NUM_TASKS       3
#define TASK_UART       0
#define TASK_LED        1
#define TASK_REPORT     2

static const char* task_names[NUM_TASKS] = {"UART", "LED", "REP"};

// Tipos de comandos para la Cola de FreeRTOS
typedef enum {
    CMD_GREEN_ON,
    CMD_GREEN_OFF,
    CMD_RED_ON,
    CMD_RED_OFF
} LedCommand_t;

QueueHandle_t ledQueue;

// Flags de Traza para la Gráfica de Línea de Tiempo en el OLED
volatile bool task_ran[NUM_TASKS] = {false, false, false};
volatile int task_pulse_counter[NUM_TASKS] = {0, 0, 0};

#define HISTORY_SIZE 96
uint8_t execution_history[HISTORY_SIZE] = {0};

volatile int command_count = 0;

// Inicialización de Periféricos de Hardware a bajo nivel
void hardware_init(void) {
    // Configuración de LEDs
    gpio_reset_pin(PIN_LED_GREEN);
    gpio_set_direction(PIN_LED_GREEN, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_LED_GREEN, 0);

    gpio_reset_pin(PIN_LED_RED);
    gpio_set_direction(PIN_LED_RED, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_LED_RED, 0);

    // Configuración de Botón (Pull-Up)
    gpio_reset_pin(PIN_BTN);
    gpio_set_direction(PIN_BTN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_BTN, GPIO_PULLUP_ONLY);
}

// Inicialización del Bus I2C Maestro nativo
void i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

// =========================================================================
// TAREA 1: Comunicación UART y Procesamiento de Comandos
// =========================================================================
void Task_UART(void *pvParameters) {
    (void) pvParameters;
    char inputBuffer[64];
    int idx = 0;

    printf("[UART Task] Lista. Ingrese comandos (ej. status, info, led green on)\r\n");

    while (1) {
        int c = getchar();
        if (c != EOF) {
            if (c == '\n' || c == '\r') {
                inputBuffer[idx] = '\0';
                if (idx > 0) {
                    command_count++;
                    task_ran[TASK_UART] = true;

                    // Intérprete de comandos básicos
                    if (strcmp(inputBuffer, "status") == 0) {
                        printf("\r\n>>> RESPUESTA STATUS:\r\n");
                        printf("  LED Verde (GPIO32): %s\r\n", gpio_get_level(PIN_LED_GREEN) ? "ENCENDIDO" : "APAGADO");
                        printf("  LED Rojo (GPIO33):  %s\r\n", gpio_get_level(PIN_LED_RED) ? "ENCENDIDO" : "APAGADO");
                        printf("  Tiempo Activo:      %llu s\r\n", esp_timer_get_time() / 1000000);
                        printf("  Total Comandos:     %d\r\n", command_count);
                    }
                    else if (strcmp(inputBuffer, "info") == 0) {
                        printf("\r\n>>> RESPUESTA INFO:\r\n");
                        printf("  Arquitectura: ESP-IDF Nativo + FreeRTOS\r\n");
                        printf("  Interfaz Grafica: Driver SSD1306 por I2C\r\n");
                        printf("  Comandos procesados: %d\r\n", command_count);
                    }
                    else if (strcmp(inputBuffer, "led green on") == 0) {
                        LedCommand_t cmd = CMD_GREEN_ON;
                        xQueueSend(ledQueue, &cmd, portMAX_DELAY);
                        printf(">>> Comando encolado: Encender LED Verde\r\n");
                    }
                    else if (strcmp(inputBuffer, "led green off") == 0) {
                        LedCommand_t cmd = CMD_GREEN_OFF;
                        xQueueSend(ledQueue, &cmd, portMAX_DELAY);
                        printf(">>> Comando encolado: Apagar LED Verde\r\n");
                    }
                    else if (strcmp(inputBuffer, "led red on") == 0) {
                        LedCommand_t cmd = CMD_RED_ON;
                        xQueueSend(ledQueue, &cmd, portMAX_DELAY);
                        printf(">>> Comando encolado: Encender LED Rojo\r\n");
                    }
                    else if (strcmp(inputBuffer, "led red off") == 0) {
                        LedCommand_t cmd = CMD_RED_OFF;
                        xQueueSend(ledQueue, &cmd, portMAX_DELAY);
                        printf(">>> Comando encolado: Apagar LED Rojo\r\n");
                    }
                    else {
                        printf(">>> Comando no reconocido: '%s'\r\n", inputBuffer);
                    }
                }
                idx = 0;
            } else {
                if (idx < (sizeof(inputBuffer) - 1)) {
                    inputBuffer[idx++] = (char)c;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// =========================================================================
// TAREA 2: Control de Hardware (LEDs y Pulsador Local)
// =========================================================================
void Task_LED(void *pvParameters) {
    (void) pvParameters;
    LedCommand_t rcvCmd;
    int last_btn_state = 1;

    while (1) {
        // Escuchar la cola de comandos enviada desde la tarea UART
        if (xQueueReceive(ledQueue, &rcvCmd, pdMS_TO_TICKS(10)) == pdTRUE) {
            task_ran[TASK_LED] = true;
            switch (rcvCmd) {
                case CMD_GREEN_ON:
                    gpio_set_level(PIN_LED_GREEN, 1);
                    break;
                case CMD_GREEN_OFF:
                    gpio_set_level(PIN_LED_GREEN, 0);
                    break;
                case CMD_RED_ON:
                    gpio_set_level(PIN_LED_RED, 1);
                    break;
                case CMD_RED_OFF:
                    gpio_set_level(PIN_LED_RED, 0);
                    break;
            }
        }

        // Lectura física local del pulsador (GPIO 13)
        int btn_state = gpio_get_level(PIN_BTN);
        if (btn_state == 0 && last_btn_state == 1) {
            vTaskDelay(pdMS_TO_TICKS(15)); // Antirrebote simple
            if (gpio_get_level(PIN_BTN) == 0) {
                task_ran[TASK_LED] = true;
                int current_state = gpio_get_level(PIN_LED_GREEN);
                gpio_set_level(PIN_LED_GREEN, !current_state);
                printf("[LED Task] Pulsador fisico presionado. Conmutando LED Verde.\r\n");
            }
        }
        last_btn_state = btn_state;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// =========================================================================
// TAREA 3: Reporte Periódico de Estado
// =========================================================================
void Task_Report(void *pvParameters) {
    (void) pvParameters;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000)); // Cada 10 segundos
        task_ran[TASK_REPORT] = true;

        printf("\r\n--- REPORTE PERIODICO DEL SISTEMA ---\r\n");
        printf("  LED Verde:     %s\r\n", gpio_get_level(PIN_LED_GREEN) ? "ENCENDIDO" : "APAGADO");
        printf("  LED Rojo:      %s\r\n", gpio_get_level(PIN_LED_RED) ? "ENCENDIDO" : "APAGADO");
        printf("  Uptime:        %llu s\r\n", esp_timer_get_time() / 1000000);
        printf("-------------------------------------\r\n");
    }
}

// =========================================================================
// TAREA 4: Renderizado de la Línea de Tiempo en el OLED (I2C Nativo)
// =========================================================================
void Task_OLED(void *pvParameters) {
    (void) pvParameters;

    // Inicializar el controlador nativo de la OLED usando los drivers provistos[cite: 1]
    if (ssd1306_init(I2C_MASTER_NUM) != ESP_OK) {
        printf("Error al inicializar la pantalla OLED por I2C\r\n");
        vTaskDelete(NULL);
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100)); // Actualizar a 10 Hz

        // Desplazar historial hacia la izquierda
        for (int i = 0; i < HISTORY_SIZE - 1; i++) {
            execution_history[i] = execution_history[i + 1];
        }

        // Evaluar banderas de actividad de las tareas
        uint8_t current_state = 0;
        for (int t = 0; t < NUM_TASKS; t++) {
            if (task_ran[t]) {
                task_pulse_counter[t] = 2; // Mantener pulso visible 2 ciclos (200ms)
                task_ran[t] = false;
            }
            if (task_pulse_counter[t] > 0) {
                current_state |= (1 << t);
                task_pulse_counter[t]--;
            }
        }
        execution_history[HISTORY_SIZE - 1] = current_state;

        // Limpiar framebuffer en RAM[cite: 1, 2]
        ssd1306_clear();

        // Dibujar texto e interfaz estática usando la fuente 5x7[cite: 1]
        ssd1306_draw_string(0, 0, "TASK TIMELINE");
        ssd1306_draw_hline(0, SSD1306_WIDTH - 1, 9, true);

        // Etiquetas de las tareas
        for (int t = 0; t < NUM_TASKS; t++) {
            ssd1306_draw_string(0, 15 + (t * 16), task_names[t]);
        }
        ssd1306_draw_vline(32, 10, SSD1306_HEIGHT - 1, true);

        // Dibujar las líneas de tiempo tipo osciloscopio
        for (int t = 0; t < NUM_TASKS; t++) {
            int baseline_y = 24 + (t * 16);
            int active_y = 14 + (t * 16);

            for (int i = 0; i < HISTORY_SIZE; i++) {
                int x = 33 + i;
                bool active = (execution_history[i] & (1 << t)) != 0;
                int y = active ? active_y : baseline_y;

                if (i > 0) {
                    bool prev_active = (execution_history[i - 1] & (1 << t)) != 0;
                    if (prev_active != active) {
                        ssd1306_draw_vline(x, active_y, active_y + 10, true);
                    }
                }
                ssd1306_set_pixel(x, y, true);
            }
        }

        // Enviar el framebuffer actualizado a la pantalla física por I2C[cite: 1, 2]
        ssd1306_update();
    }
}

// =========================================================================
// FUNCIÓN PRINCIPAL app_main (ESP-IDF)
// =========================================================================
void app_main(void) {
    printf("\n==========================================\r\n");
    printf("   TAREA 5 - SISTEMAS EMBEBIDOS (ESP-IDF) \r\n");
    printf("   EJERCICIO 3: SISTEMA INTEGRADO (I2C/UART)\r\n");
    printf("==========================================\r\n");

    hardware_init();
    i2c_master_init();

    // Crear la Cola de Mensajes de FreeRTOS
    ledQueue = xQueueCreate(10, sizeof(LedCommand_t));
    if (ledQueue == NULL) {
        printf("Error crítico: No se pudo crear ledQueue\r\n");
        return;
    }

    // Creación de Tareas FreeRTOS con xTaskCreatePinnedToCore
    xTaskCreatePinnedToCore(Task_UART,   "Task_UART",   3072, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(Task_LED,    "Task_LED",    3072, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(Task_Report, "Task_Report", 3072, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(Task_OLED,   "Task_OLED",   4096, NULL, 2, NULL, 1);

    printf("Todas las tareas del RTOS han sido inicializadas correctamente.\r\n");
}