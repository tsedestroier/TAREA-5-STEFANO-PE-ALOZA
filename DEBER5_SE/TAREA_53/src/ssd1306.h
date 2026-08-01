#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c.h"

#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  64
#define SSD1306_ADDR    0x3C   // Direccion I2C tipica de los modulos 0.96" (algunos usan 0x3D)

// Inicializa el driver sobre el puerto I2C indicado (que ya debe estar
// instalado con i2c_driver_install antes de llamar a esta funcion).
esp_err_t ssd1306_init(i2c_port_t port);

// Limpia el framebuffer en RAM (no envia nada a la pantalla todavia).
void ssd1306_clear(void);

// Prende/apaga un pixel individual en el framebuffer.
void ssd1306_set_pixel(int x, int y, bool on);

// Lineas horizontales / verticales (utiles para las barras del diagrama).
void ssd1306_draw_hline(int x0, int x1, int y, bool on);
void ssd1306_draw_vline(int x, int y0, int y1, bool on);

// Linea vertical punteada (para las marcas de tiempo tipo t1,t2,t3).
void ssd1306_draw_vline_dashed(int x, int y0, int y1);

// Texto con la fuente 5x7 (subset de caracteres, ver font5x7.h).
void ssd1306_draw_char(int x, int y, char c);
void ssd1306_draw_string(int x, int y, const char *s);

// Vuelca el framebuffer completo a la pantalla fisica por I2C.
esp_err_t ssd1306_update(void);
