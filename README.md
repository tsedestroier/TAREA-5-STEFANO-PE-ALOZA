# 🚀 Tarea: Sistemas Embebidos (ESP32 + FreeRTOS + I2C + UART)

Repositorio oficial con la implementación de los ejercicios prácticos desarrollados para la placa ESP32, simulados en **Wokwi** y compilados mediante **PlatformIO**. La estructura principal se encuentra organizada dentro de la carpeta **`DEBER5_SE/`**.

---

## 📂 Índice y Acceso Rápido a los Ejercicios

Haz clic en cualquier ejercicio para revisar directamente su código fuente:

* **[Ejercicio 1: Sistema de Comandos por UART2](./DEBER5_SE/TAREA_5/)**
* **[Ejercicio 2: Multitarea con FreeRTOS](./DEBER5_SE/TAREA_52/)**
* **[Ejercicio 3: Sistema Integrado (UART + FreeRTOS + I2C + OLED)](./DEBER5_SE/TAREA_53/)**

---

## 🧠 Explicación Detallada del Funcionamiento del Código

### 1. [Ejercicio 1: Sistema de Comandos por UART2](./DEBER5_SE/TAREA_5/)
* **Funcionamiento técnico:** Este módulo implementa un intérprete de comandos robusto utilizando exclusivamente **UART2** del ESP32 con los drivers nativos de ESP-IDF (`driver/uart.h`).
* **Lectura no bloqueante:** Emplea un búfer de recepción y un tiempo de espera controlado (`uart_read_bytes`) que evita que el microcontrolador se quede congelado esperando datos, permitiendo que el bucle principal ceda tiempo de procesamiento al sistema operativo.
* **Comandos soportados:**
  * `status`: Devuelve un reporte completo del estado del sistema, salud de los procesos, estado actual del LED y el tiempo activo (*uptime*).
  * `led on` / `led off`: Controla de manera directa el nivel lógico y físico del pin del LED.
  * `info`: Proporciona detalles técnicos de la interfaz (baudios, puerto y contador de comandos procesados).
  * `reset`: Reinicia las variables internas y contadores lógicos de manera segura sin reiniciar el microcontrolador.

### 2. [Ejercicio 2: Multitarea con FreeRTOS](./DEBER5_SE/TAREA_52/)
* **Funcionamiento técnico:** Se enfoca en la distribución de cargas de trabajo concurrentes utilizando el núcleo de tiempo real **FreeRTOS**.
* **Estrategia de hilos:** Divide las responsabilidades del sistema en tareas independientes ejecutadas de manera paralela, priorizando el uso de recursos y asegurando que las rutinas de lectura de sensores y control no interfieran con la comunicación serie.

### 3. [Ejercicio 3: Sistema Integrado (UART + FreeRTOS + I2C + OLED)](./DEBER5_SE/TAREA_53/)
* **Funcionamiento técnico:** Es el proyecto integrador que unifica todos los conceptos anteriores en un entorno multitarea avanzado.
* **Distribución en Núcleos (`xTaskCreatePinnedToCore`):** Las tareas están repartidas de forma óptima entre los núcleos del ESP32:
  * **Tarea UART:** Captura de forma asíncrona los comandos ingresados por el usuario.
  * **Tarea LED / Hardware:** Gestiona la cola de mensajes (*Queue*) y permite conmutar el estado del LED tanto por comandos virtuales como mediante un **pulsador físico local (GPIO 13)** con lógica antirrebote.
  * **Tarea de Reporte:** Realiza la lectura analógica del potenciómetro y emite reportes periódicos automatizados.
  * **Tarea OLED (I2C):** Controla la pantalla mediante los drivers nativos del controlador **SSD1306** (`ssd1306.c` / `ssd1306.h`). Renderiza en tiempo real una **gráfica de líneas de tiempo estilo osciloscopio (T1 a T4)** para visualizar la actividad dinámica de los hilos, protegiendo el intercambio de datos mediante semáforos tipo **Mutex**.

---

## 🛠️ Requisitos Técnicos y Entorno
* **Hardware simulado:** ESP32 DevKit V1, pantalla OLED SSD1306 (128x64), potenciómetro y LEDs indicadores.
* **Entorno de desarrollo:** Visual Studio Code con **PlatformIO IDE**.
* **Frameworks:** Arduino / ESP-IDF.

---

## ⚙️ Instrucciones de Ejecución
1. Clona este repositorio o descárgalo en tu equipo.
2. Abre la carpeta del proyecto en **Visual Studio Code**.
3. Asegúrate de tener instalada la extensión de **PlatformIO**.
4. Compila y carga el proyecto utilizando las opciones de **Clean** y **Build** de PlatformIO.
