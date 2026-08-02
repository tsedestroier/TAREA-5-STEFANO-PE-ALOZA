# Tarea: Sistemas Embebidos (ESP32 + FreeRTOS + I2C + UART)

**Estudiante:** Stefano Peñaloza  
**Enlace al Repositorio de GitHub:** [*(https://github.com/tsedestroier/TAREA-5-STEFANO-PE-ALOZA/blob/main/README.md)*]  
**Simulación en Wokwi:** [Ver Simulación](https://wokwi.com/projects/470989175279998977)  
**Video de Demostración:** [Ver en YouTube](https://youtu.be/I-_zX-c1B3I)

---

## 📋 Introducción General
Este documento técnico recopila la implementación y el análisis de los sistemas embebidos desarrollados para la placa ESP32, enfocados en la comunicación serial avanzada, la gestión de periféricos de hardware mediante drivers nativos, la concurrencia en tiempo real con FreeRTOS y la integración de interfaces visuales por bus I2C. A continuación, se detalla exhaustivamente el funcionamiento de cada ejercicio.

---

## ⚙️ EJERCICIO 1: Sistema de Comandos por UART2
**[📁 Ver código fuente del Ejercicio 1](./DEBER5_SE/TAREA_5/)**

### 1. Descripción del Ejercicio
El Ejercicio 1 implementa un sistema robusto de comunicación serial basado en el puerto **UART2** del microcontrolador ESP32 utilizando los drivers nativos de **ESP-IDF**. El sistema opera como un intérprete de comandos interactivo capaz de recibir cadenas de texto de forma no bloqueante a través de un búfer dedicado, procesar la solicitud del usuario, controlar un pin de propósito general (GPIO) vinculado a un LED y retornar respuestas formateadas sin comprometer ciclos de procesamiento del sistema.

### 2. Explicación del Funcionamiento del Sistema
El funcionamiento del firmware se divide en dos fases principales: la inicialización de los controladores y el bucle operativo asíncrono.

* **Inicialización de Periféricos:** Al arrancar el sistema dentro de `app_main`, se configuran los pines físicos de transmisión y recepción (`GPIO 17` para TX y `GPIO 16` para RX) junto con el controlador UART2 a una velocidad de 115200 baudios. Asimismo, se inicializa el pin del LED como salida digital en estado bajo.
* **Lectura No Bloqueante:** En el núcleo del bucle infinito, la función `uart_read_bytes` se ejecuta con un tiempo de espera (*timeout*) de 100 ms. Esto permite que el ESP32 busque datos en el búfer serial sin bloquear la ejecución si no hay tramas disponibles, permitiendo la integración de retardos de control de rendimiento (`vTaskDelay`).
* **Intérprete y Filtrado de Comandos:** Cuando se reciben bytes, se añade un terminador nulo para asegurar la compatibilidad con cadenas de caracteres en C (`strings`). La función `strip_newline` se encarga de eliminar los caracteres de retorno de carro (`\r`) o salto de línea (`\n`). Posteriormente, la función `process_command` compara la cadena ingresada mediante `strcmp` y direcciona la lógica hacia el comando correspondiente (`status`, `led on`, `led off`, `info` o `reset`), incrementando el contador global de comandos y emitiendo una respuesta estructurada de vuelta por el canal UART2.

---

### 3. Diagrama de Flujo y Arquitectura del Sistema

```text
       [ INICIO: app_main() ]
                 │
                 ▼
     [ Configurar UART2 y GPIO LED ]
                 │
                 ▼
       ┌──► [ BUCLE PRINCIPAL ]
       │         │
       │         ├─► ¿Llegaron datos por UART2? (Timeout: 100ms)
       │         │     ├── SÍ ──► [ Leer bytes y limpiar saltos de línea ]
       │         │     │               │
       │         │     │               ▼
       │         │     │        [ process_command() ]
       │         │     │               │
       │         │     │               ├─► "status" ──► Enviar reporte detallado
       │         │     │               ├─► "led on" ──► Encender LED (GPIO 2)
       │         │     │               ├─► "led off" ─► Apagar LED (GPIO 2)
       │         │     │               ├─► "info" ────► Mostrar baudios y contador
       │         │     │               ├─► "reset" ───► Reiniciar variables lógicas
       │         │     │               └─► OTROS ─────► Enviar error de comando
       │         │     │
       │         │     └── NO ──┐
       │         │              │
       │         ▼              ▼
       │   [ vTaskDelay(10ms) ] ◄─────┘
       │         │
       └─────────┘
