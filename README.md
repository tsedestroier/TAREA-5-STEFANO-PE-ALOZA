# Tarea: Sistemas Embebidos (ESP32 + FreeRTOS + I2C + UART)

**Estudiante:** Stefano Peñaloza  
**Enlace al Repositorio de GitHub:** [*(Insertar aquí el enlace a tu repositorio público de GitHub)*]  
**Simulación en Wokwi:** [Ver Simulación](https://wokwi.com/projects/470989175279998977)  
**Video de Demostración:** [Ver en YouTube](https://youtu.be/I-_zX-c1B3I)

---

## 📋 Introducción General
Este documento técnico recopila la implementación y el análisis de los sistemas embebidos desarrollados para la placa ESP32, enfocados en la comunicación serial avanzada, la gestión de periféricos de hardware mediante drivers nativos, la concurrencia en tiempo real con FreeRTOS y la integración de interfaces visuales por bus I2C. A continuación, se detalla exhaustivamente el **Ejercicio 1**.

---

## ⚙️ EJERCICIO 1: Sistema de Comandos por UART2

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



### 📊 Variables Clave del Sistema

| Variable | Tipo | Alcance | Descripción |
| :--- | :--- | :--- | :--- |
| `command_counter` | `int` | Global | Almacena el número total de comandos válidos procesados desde el arranque o el último reseteo. |
| `led_state` | `bool` | Global | Bandera lógica que refleja el estado actual del LED (`true` encendido, `false` apagado). |
| `system_status_ok` | `bool` | Global | Indicador de la salud general del sistema de comandos (`true` para óptimo, `false` en error). |
| `rx_data` | `uint8_t[]` | Local | Búfer de memoria estática encargado de almacenar temporalmente los bytes crudos leídos del puerto serie. |

### 🛠️ Funciones Principales

| Función | Parámetros | Retorno | Descripción Detallada |
| :--- | :--- | :--- | :--- |
| `uart_config_custom` | `uart_port_t, int, ...` | `void` | Borra la configuración previa del UART, aplica baudios, paridad y reasigna pines TX/RX. |
| `led_config` | `void` | `void` | Resetea el pin del LED, lo configura como salida digital y establece su nivel inicial en cero. |
| `send_uart_response` | `const char*` | `void` | Toma una cadena de texto, calcula su longitud y la escribe por UART2 con salto de línea. |
| `strip_newline` | `char*` | `void` | Analiza el final de la cadena de caracteres y remueve de forma segura los caracteres `\r` o `\n`. |
| `process_command` | `char*` | `void` | Núcleo de control lógico; limpia la cadena, incrementa contadores y evalúa las acciones del hardware. |
