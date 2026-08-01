# TAREA-5-STEFANO-PEÑALOZA
Este repositorio corresponde a la tarea 5 de sistemas embedidos

# 🚀 Tarea: Sistemas Embebidos (ESP32 + FreeRTOS + I2C + UART)

Repositorio oficial con la implementación de los ejercicios prácticos desarrollados con **ESP-IDF**, **Arduino Framework** y **FreeRTOS** para la placa ESP32, simulados en **Wokwi** y compilados mediante **PlatformIO**.

---

## 📂 Índice y Acceso Rápido a los Ejercicios

Haz clic en cualquier ejercicio para revisar directamente su código fuente:

* **[Ejercicio 1: Sistema de Comandos por UART2](./TAREA_5/)**
  * *Descripción:* Implementación de comunicación serie mediante UART2 usando drivers nativos de ESP-IDF, lectura no bloqueante con buffers y procesamiento de comandos (`status`, `led on`, `led off`, `info`, `reset`).
* **[Ejercicio 2: Multitarea con FreeRTOS](./TAREA_52/)**
  * *Descripción:* Creación y distribución de tareas concurrentes utilizando FreeRTOS en múltiples núcleos del ESP32.
* **[Ejercicio 3: Sistema Integrado (UART + FreeRTOS + I2C + OLED)](./TAREA_53/)**
  * *Descripción:* Proyecto integrador que combina la recepción de comandos por UART, control de hardware (LEDs y pulsador físico), lectura de sensores analógicos y renderizado en tiempo real de una gráfica de líneas de tiempo en una pantalla **OLED SSD1306** por I2C.

---

## 🛠️ Requisitos Técnicos y Entorno
* **Hardware simulado:** ESP32 DevKit V1, pantalla OLED SSD1306 (128x64), potenciómetro y LEDs indicadores.
* **Entorno de desarrollo:** Visual Studio Code con **PlatformIO IDE**.
* **Frameworks:** Arduino / ESP-IDF (según el requerimiento del ejercicio).

---

## ⚙️ Instrucciones de Ejecución
1. Clona este repositorio o descárgalo en tu equipo.
2. Abre la carpeta del proyecto en **Visual Studio Code**.
3. Asegúrate de tener instalada la extensión de **PlatformIO**.
4. Compila y carga el proyecto utilizando las opciones de **Clean** y **Build** de PlatformIO.
