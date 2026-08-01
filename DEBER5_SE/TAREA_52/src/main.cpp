#include <Arduino.h>

#include <Wire.h>

#include <Adafruit_GFX.h>

#include <Adafruit_SSD1306.h>



// Definiciones de Hardware
  
#define pinLED2 32  // LED Verde

#define pinLed  33  // LED Rojo

#define pinBTN  13  // Pulsador (disponible en la placa)

#define pot     34  // Potenciómetro



// Configuración de la pantalla OLED (SSD1306)

#define SCREEN_WIDTH 128

#define SCREEN_HEIGHT 64

#define OLED_RESET -1

#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



// Variable global compartida para almacenar la lectura del sensor

volatile int pot_percent = 0;



// Prototipos de Tareas de FreeRTOS

void Task_LED_Green(void *pvParameters);

void Task_LED_Red(void *pvParameters);

void Task_Sensor(void *pvParameters);

void Task_Report_Serial(void *pvParameters);

void Task_OLED_Display(void *pvParameters);



void setup() {

  // Inicializar comunicación UART

  Serial.begin(115200);

  delay(500);



  // Se imprime en el arranque del microcontrolador

  Serial.println("\n=====================================================================");

  Serial.println("  TAREA 5 - EJERCICIO 2: SISTEMA MULTITAREA CON FreeRTOS (ESP32)     ");




  // Inicializar I2C y pantalla OLED

  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {

    Serial.println(F("Error: No se pudo asignar memoria para SSD1306"));

    for (;;);

  }



  // Pantalla de inicio

  display.clearDisplay();

  display.setTextSize(1);

  display.setTextColor(SSD1306_WHITE);

  display.setCursor(15, 10);

  display.println("Sistemas Embebidos");

  display.setCursor(20, 25);

  display.println("EJERCICIO #2");

  display.setCursor(10, 45);

  display.println("Iniciando RTOS...");

  display.display();

  delay(1500);



  // Configuración de Pines GPIO

  pinMode(pinLED2, OUTPUT);

  pinMode(pinLed, OUTPUT);

  pinMode(pinBTN, INPUT_PULLUP);

  pinMode(pot, INPUT);



  // Crear Tareas concurrentes en FreeRTOS

  // Las tareas de los LEDs tienen mayor prioridad para garantizar tiempos exactos.

  xTaskCreatePinnedToCore(Task_LED_Green,   "Task_LED_Green",   2048, NULL, 2, NULL, 1);

  xTaskCreatePinnedToCore(Task_LED_Red,     "Task_LED_Red",     2048, NULL, 2, NULL, 1);

  xTaskCreatePinnedToCore(Task_Sensor,      "Task_Sensor",      2048, NULL, 1, NULL, 1);

  xTaskCreatePinnedToCore(Task_Report_Serial,"Task_Report",      3072, NULL, 1, NULL, 1);

  xTaskCreatePinnedToCore(Task_OLED_Display,"Task_OLED",        4096, NULL, 1, NULL, 1);



  Serial.println("Tareas de FreeRTOS inicializadas correctamente.");

}



void loop() {

  // El ciclo loop tradicional permanece inactivo.

  // Toda la logica corre en las tareas concurrentes creadas arriba.

  vTaskDelay(pdMS_TO_TICKS(1000));

}



// TAREA 1: Parpadeo del LED Verde (Frecuencia de 500 ms)

void Task_LED_Green(void *pvParameters) {

  (void) pvParameters;



  while (1) {

    digitalWrite(pinLED2, !digitalRead(pinLED2));

    vTaskDelay(pdMS_TO_TICKS(500)); // Espera 500ms suspendiendo la tarea

  }

}



// TAREA 2: Parpadeo del LED Rojo (Frecuencia de 1000 ms)

void Task_LED_Red(void *pvParameters) {

  (void) pvParameters;



  while (1) {

    digitalWrite(pinLed, !digitalRead(pinLed));

    vTaskDelay(pdMS_TO_TICKS(1000)); // Espera 1000ms suspendiendo la tarea

  }

}



// TAREA 3: Lectura del potenciómetro (Sensor virtual)

void Task_Sensor(void *pvParameters) {

  (void) pvParameters;



  while (1) {

    int raw_val = analogRead(pot);

    // Convertir la lectura de 12 bits (0-4095) a porcentaje (0-100)

    pot_percent = map(raw_val, 0, 4095, 0, 100);

    

    vTaskDelay(pdMS_TO_TICKS(500)); // Muestrear cada 500ms

  }

}



// TAREA 4: Reporte periódico al Monitor Serial

void Task_Report_Serial(void *pvParameters) {
// TAREA 4: Reporte periódico al Monitor Serial
  (void) pvParameters;

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(2000)); // Reporta cada 2 segundos

    Serial.println(">>> [FreeRTOS Reporte de Tareas]");
    Serial.printf("  Uptime:       %lu s\r\n", millis() / 1000);
    Serial.printf("  Sensor (Pot): %d%%\r\n", pot_percent);
    Serial.printf("  LED Verde:    %s (Parpadeo: 500ms)\r\n", digitalRead(pinLED2) ? "ENCENDIDO" : "APAGADO");
    Serial.printf("  LED Rojo:     %s (Parpadeo: 1000ms)\r\n", digitalRead(pinLed) ? "ENCENDIDO" : "APAGADO");
    Serial.println("-------------------------------------");
}
}



// TAREA 5: Visualización de información y barra de progreso en la pantalla OLED

void Task_OLED_Display(void *pvParameters) {

  (void) pvParameters;



  while (1) {

    display.clearDisplay();



    // Título de la interfaz

    display.setTextSize(1);

    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);

    display.print("FreeRTOS Multitasking");

    display.drawFastHLine(0, 9, SCREEN_WIDTH, SSD1306_WHITE);



    // Valor del Potenciómetro

    display.setCursor(5, 12);

    display.printf("Sensor (Pot): %d%%", pot_percent);



    // Barra de progreso del potenciómetro

    display.drawRect(5, 22, 118, 10, SSD1306_WHITE);

    int fill_width = map(pot_percent, 0, 100, 0, 114);

    if (fill_width > 0) {

      display.fillRect(7, 24, fill_width, 6, SSD1306_WHITE);

    }



    // Estado del LED Verde

    display.setCursor(5, 38);

    display.print("LED Verde (500ms):");

    if (digitalRead(pinLED2)) {

      display.fillCircle(115, 41, 4, SSD1306_WHITE); // Círculo relleno si está ON

    } else {

      display.drawCircle(115, 41, 4, SSD1306_WHITE); // Círculo vacío si está OFF

    }



    // Estado del LED Rojo

    display.setCursor(5, 50);

    display.print("LED Rojo (1000ms):");

    if (digitalRead(pinLed)) {

      display.fillCircle(115, 53, 4, SSD1306_WHITE); // Círculo relleno si está ON

    } else {

      display.drawCircle(115, 53, 4, SSD1306_WHITE); // Círculo vacío si está OFF

    }



    // Mostrar en pantalla

    display.display();



    vTaskDelay(pdMS_TO_TICKS(200)); // Actualizar pantalla a 5Hz

  }

} 

