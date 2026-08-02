# Tarea: Sistemas Embebidos (ESP32 + FreeRTOS + I2C + UART)

**Estudiante:** Stefano Peñaloza  
**Enlace al Repositorio de GitHub:** [*https://github.com/tsedestroier/TAREA-5-STEFANO-PE-ALOZA/blob/main/README.md*]  
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
       │         │
       │         ├─► ¿Llegaron datos por UART2? (Timeout: 100ms)
       │         │     ├── SÍ ──► [ Leer bytes y limpiar saltos de línea ]
       │         │     │               │
       │         │     │               ▼
       │         │     │        [ process_command() ]
       │         │     │               │
       │         │     │               ├─► "status" ──► Enviar reporte detallado
       │         │     │               ├─► "led on" ──► Encender LED (GPIO 2)
       │         │     │               ├─► "led off" ─► Apagar LED (GPIO 2)
       │         │     │               ├─► "info" ────► Mostrar baudios y contador
       │         │     │               ├─► "reset" ───► Reiniciar variables lógicas
       │         │     │               └─► OTROS ─────► Enviar error de comando
       │         │     │
       │         │     └── NO ──┐
       │         │              │
       │         ▼              ▼
       │   [ vTaskDelay(10ms) ] ◄─────┘
       │         │
       └─────────┘



## 4. Variables Clave del Sistema

- `command_counter` (`int`, Global): Almacena el número total de comandos válidos procesados desde el arranque o el último reseteo.

- `led_state` (`bool`, Global): Bandera lógica que refleja el estado actual del LED (`true` si está encendido, `false` si está apagado).

- `system_status_ok` (`bool`, Global): Indicador de la salud general del sistema de comandos (`true` para óptimo, `false` en error).

- `rx_data` (`uint8_t[]`, Local en `app_main`): Búfer de memoria estática encargado de almacenar temporalmente los bytes crudos leídos del puerto serie.

## 5. Funciones Principales y su Propósito

- `uart_config_custom`: Borra la configuración previa del UART especificado, aplica los parámetros de baudios, paridad, bits de parada y reasigna los pines físicos TX/RX mediante ESP-IDF.

- `led_config`: Resetea el pin del LED mediante el driver GPIO, lo configura explícitamente como salida digital y establece su nivel inicial en cero.

- `send_uart_response`: Toma una cadena de texto, calcula su longitud y la escribe a través del canal UART2 agregando automáticamente un retorno de carro y salto de línea (`\r\n`).

- `strip_newline`: Analiza el final de la cadena de caracteres recibida y remueve de forma segura los caracteres invisibles de fin de línea (`\r`, `\n`) para permitir una comparación limpia.

- `process_command`: Núcleo de control lógico; limpia la cadena, incrementa el contador de comandos y evalúa mediante bloques condicionales qué acción ejecutar sobre el hardware o las variables internas.


---

## ⚙️ EJERCICIO 2: Multitarea con FreeRTOS

### 1. Descripción del Ejercicio
El Ejercicio 2 se centra en la implementación de un Sistema Operativo de Tiempo Real (RTOS) utilizando **FreeRTOS** sobre la arquitectura del ESP32. El objetivo principal es abandonar el enfoque secuencial tradicional (el clásico *super-loop* o bucle infinito único) y transicionar hacia un modelo concurrente donde múltiples tareas o hilos de ejecución operan en paralelo, gestionados por el planificador (*scheduler*) del sistema operativo según sus prioridades asignadas.

### 2. Explicación del Funcionamiento del Sistema
El funcionamiento del sistema radica en la creación y distribución de tres tareas independientes, cada una con un propósito, prioridad y tiempo de ejecución específico para evitar bloqueos del procesador (*starvation*):

*   **Inicialización del Sistema:** En la función principal `app_main`, se configuran los periféricos base (como los pines de los LEDs o sensores). Inmediatamente después, se invocan las APIs de FreeRTOS (`xTaskCreate` o `xTaskCreatePinnedToCore`) para registrar las tareas en el sistema, asignándoles memoria de pila (*stack*), prioridad y un núcleo de ejecución.
*   **Gestión Concurrente:** Una vez creadas, el *scheduler* de FreeRTOS toma el control. 
    *   Una tarea se encarga del control de hardware (ej. parpadeo de un LED para indicar que el sistema está vivo).
    *   Otra tarea se encarga de la lectura periódica de periféricos (ej. estado de pulsadores o sensores analógicos).
    *   Una tercera tarea gestiona la impresión de diagnósticos a través del puerto serial.
*   **Control de Tiempos no Bloqueante:** A diferencia de la función `delay()` convencional que congela el procesador, cada tarea utiliza `vTaskDelay()`. Esto indica al planificador que la tarea actual pasa a un estado de bloqueo temporal (*Blocked state*), cediendo instantáneamente el tiempo de CPU a otras tareas de menor o igual prioridad que estén listas para ejecutarse (*Ready state*).

---

### 3. Diagrama de Flujo y Arquitectura del Sistema

```text
               [ INICIO: app_main() ]
                         │
                         ▼
        [ Configuración de Periféricos (GPIO) ]
                         │
                         ▼
             [ Planificador FreeRTOS ] ◄──────────────┐
                         │                            │
      ┌──────────────────┼──────────────────┐         │ (Asignación 
      ▼                  ▼                  ▼         │  de CPU)
 [ TAREA 1 ]        [ TAREA 2 ]        [ TAREA 3 ]    │
(Control LED)      (Lectura I/O)      (Comunicación)  │
      │                  │                  │         │
      ▼                  ▼                  ▼         │
 [ Acción ]         [ Lectura ]        [ Enviar ]     │
      │                  │                  │         │
      ▼                  ▼                  ▼         │
[ vTaskDelay ]     [ vTaskDelay ]     [ vTaskDelay ]  │
      │                  │                  │         │
      └──────────────────┴──────────────────┴─────────┘
           (Ceden el control al Scheduler)

## 4. Variables Clave del Sistema

- `task_led_handle` (`TaskHandle_t`, Global): Puntero referencial que almacena el identificador de la tarea encargada del parpadeo del LED, útil si se requiere suspender o reanudar la tarea desde otra sección del código.

- `task_sensor_handle` (`TaskHandle_t`, Global): Referencia a la tarea dedicada a la adquisición de datos de los periféricos de entrada.

- `task_serial_handle` (`TaskHandle_t`, Global): Referencia a la tarea de menor prioridad encargada de enviar datos por la consola UART.

- `LED_PIN` (`int`, Constante): Define el pin físico de la placa ESP32 (usualmente GPIO 2) utilizado para la retroalimentación visual de la Tarea 1.

## 5. Funciones Principales y su Propósito

- `xTaskCreatePinnedToCore`: Función nativa de FreeRTOS (adaptada por ESP-IDF) que instancia una nueva tarea. Define la función a ejecutar, el nombre de la tarea, la profundidad de la pila en palabras (*stack size*), los parámetros a pasar, la prioridad (0 es la más baja) y el núcleo del procesador (0 o 1).

- `vTaskDelay`: API fundamental de FreeRTOS que suspende la ejecución de la tarea que la invoca durante un número específico de *ticks* del sistema. Se suele combinar con `portTICK_PERIOD_MS` para convertir milisegundos reales en *ticks*.

- `vTask1_ControlLED`: Función infinita (`while(1)`) que encapsula la lógica de la primera tarea; conmuta el nivel lógico del pin del LED y entra en reposo.

- `vTask2_LecturaDatos`: Función infinita que muestrea los puertos de entrada a una frecuencia determinada, procesando la información sin afectar la temporización del LED.

- `vTask3_ReporteSerial`: Función infinita encargada de recopilar el estado del sistema y transmitirlo vía `printf` o UART. Al manejar I/O, suele tener la prioridad más baja para no retrasar tareas críticas.


---

## ⚙️ EJERCICIO 3: Sistema Integrado (UART + FreeRTOS + I2C + OLED)

### 1. Descripción del Ejercicio
El Ejercicio 3 es un proyecto integrador que unifica los conceptos de comunicación asíncrona, sistemas operativos de tiempo real y control de periféricos complejos. Implementa un firmware multitarea donde el microcontrolador ESP32 gestiona simultáneamente la recepción de comandos por UART, la lectura de un potenciómetro (ADC), el control de actuadores físicos interactuando con un pulsador (con lógica antirrebote), y la visualización de datos en tiempo real mediante una pantalla OLED (controlador SSD1306) utilizando el bus I2C. Para asegurar la integridad del sistema, los recursos compartidos se protegen mediante colas de mensajes y semáforos de exclusión mutua (Mutex).

### 2. Explicación del Funcionamiento del Sistema
El sistema se fundamenta en una arquitectura concurrente avanzada dividida en tareas específicas, operando bajo el planificador de FreeRTOS:

*   **Gestión de Hardware y Antirrebote:** Se configuran los pines de E/S. El sistema lee continuamente un pulsador físico que permite alternar estados locales. Para evitar lecturas falsas causadas por la imperfección mecánica del botón, se implementa un filtro de software por tiempo (*debouncing*).
*   **Comunicación Segura Inter-Procesos (IPC):** Dado que múltiples tareas se ejecutan a la vez, se utilizan primitivas de FreeRTOS para evitar colisiones (*Race Conditions*). El valor del ADC o el estado lógico de los LEDs son protegidos mediante un **Mutex**. Una tarea debe "tomar" el Mutex antes de modificar o leer un dato, impidiendo que otras tareas interfieran hasta que el Mutex sea liberado.
*   **Interfaz Gráfica (OLED por I2C):** Una tarea dedicada de alta prioridad gestiona el controlador I2C para comunicarse con la pantalla SSD1306 (128x64 píxeles). Esta tarea lee periódicamente los valores registrados del ADC, limpia el búfer de video, y dibuja de forma dinámica una gráfica de línea de tiempo estilo osciloscopio (T1 a T4) que se desplaza actualizando la pantalla en tiempo real sin congelar la recepción de comandos seriales.
*   **Recepción asíncrona:** La tarea UART trabaja en segundo plano. Si recibe un comando válido (ej. apagar LED, resetear cuenta), envía una instrucción a través de una **Cola de Mensajes (*Queue*)** hacia la tarea controladora principal, desligando el hardware de lectura del hardware de procesamiento.

---

### 3. Diagrama de Flujo y Arquitectura del Sistema

```text
               [ Entradas Físicas ]          [ Entrada Serial ]
             (Botón GPIO)   (ADC/Pot)           (UART RX)
                  │             │                   │
                  ▼             ▼                   ▼
            ┌─────────────────────────────────────────────┐
            │            PLANIFICADOR FREERTOS            │
            │                                             │
            │  [ Tarea 1: Control I/O ] ◄──┐ (Cola de     │
            │            │                 │  Mensajes)   │
            │            ▼                 │              │
            │  [ Tarea 2: Lectura ADC ]    ├─ [ Tarea 3: CLI UART ]
            │            │                 │              │
            │            ▼ (Datos vía      │              │
            │              Mutex)          │              │
            │  [ Tarea 4: Render OLED ] ◄──┘              │
            └────────────┬────────────────────────────────┘
                         │
                         ▼ (Comunicación Bus I2C)
                 [ Pantalla OLED SSD1306 ]
                 (Renderizado de Gráfica)

### 4. Variables Clave del Sistema

* **`xMutex_Datos`** (`SemaphoreHandle_t`, Global): Semáforo de exclusión mutua utilizado para bloquear y proteger el acceso a las variables críticas (estado del sistema, valor ADC) compartidas entre las diferentes tareas.
* **`adc_value`** (`int`, Global/Protegida): Almacena la lectura digitalizada actual del potenciómetro, la cual es actualizada por la tarea de sensores y consumida por la tarea de renderizado OLED.
* **`comando_queue`** (`QueueHandle_t`, Global): Cola de mensajes que transfiere de manera segura las instrucciones decodificadas desde la tarea UART hacia la tarea de control general.
* **`oled_buffer`** (`uint8_t[]`, Local en tarea OLED): Arreglo de memoria que representa el mapa de bits (lienzo gráfico) de la pantalla, donde se calculan y dibujan los píxeles de las líneas de tiempo antes de su transmisión I2C.

---

### 5. Funciones Principales y su Propósito

* **`i2c_master_init`**: Configura los pines físicos (SDA, SCL), la velocidad del reloj y los parámetros del maestro I2C requeridos por ESP-IDF para establecer un canal de comunicación estable con el controlador SSD1306.
* **`ssd1306_display_text` / `ssd1306_draw_line`**: Funciones del driver de la pantalla encargadas de mapear cadenas de caracteres o trazar líneas mediante coordenadas cartesianas (X, Y) dentro de la matriz de la pantalla OLED.
* **`xSemaphoreTake` / `xSemaphoreGive`**: Funciones nativas de FreeRTOS de bloqueo y liberación implementadas alrededor de las variables globales. Garantizan que, por ejemplo, la tarea OLED no lea una variable ADC partida a la mitad mientras otra tarea la está reescribiendo.
* **`xQueueSend` / `xQueueReceive`**: APIs de manipulación de colas. Permiten inyectar comandos desde UART y poner en pausa (estado de bloqueo) a la tarea receptora hasta que llegue un nuevo paquete de información válido.
* **`adc1_get_raw`**: Invoca al controlador nativo del Convertidor Analógico-Digital (ADC) del ESP32 para procesar y devolver una muestra cruda del nivel de voltaje presente en el pin del potenciómetro.


este es mi codigo completo, revisa coherencia y tambien incluye hipervinculo a las carpetas de los ejercicios en cada caso, recuerda que este es mi github
