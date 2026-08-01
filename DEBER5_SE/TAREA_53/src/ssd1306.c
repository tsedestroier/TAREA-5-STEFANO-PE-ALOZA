#include "ssd1306.h"
#include "font5x7.h"
#include <string.h>

// Framebuffer en "paginas": 8 filas de 8 pixeles verticales cada una,
// igual que la memoria interna de la SSD1306 (128 columnas x 8 paginas).
static uint8_t s_fb[SSD1306_WIDTH * (SSD1306_HEIGHT / 8)];
static i2c_port_t s_port;

static esp_err_t ssd1306_cmd(uint8_t cmd)
{
    uint8_t buf[2] = { 0x00, cmd }; // 0x00 = byte de control (comando)
    return i2c_master_write_to_device(s_port, SSD1306_ADDR, buf, sizeof(buf),
                                       pdMS_TO_TICKS(100));
}

esp_err_t ssd1306_init(i2c_port_t port)
{
    s_port = port;

    static const uint8_t init_cmds[] = {
        0xAE,       // Display OFF
        0xD5, 0x80, // Clock divide
        0xA8, 0x3F, // Multiplex ratio 64
        0xD3, 0x00, // Display offset
        0x40,       // Start line 0
        0x8D, 0x14, // Charge pump ON
        0x20, 0x00, // Memory addressing: horizontal
        0xA1,       // Segment remap (espejo horizontal)
        0xC8,       // COM scan direccion invertida (espejo vertical)
        0xDA, 0x12, // COM pins
        0x81, 0x7F, // Contraste
        0xD9, 0xF1, // Precharge
        0xDB, 0x40, // VCOMH
        0xA4,       // Resume to RAM content
        0xA6,       // Normal display (no invertido)
        0xAF,       // Display ON
    };

    for (size_t i = 0; i < sizeof(init_cmds); i++) {
        esp_err_t err = ssd1306_cmd(init_cmds[i]);
        if (err != ESP_OK) {
            return err;
        }
    }

    ssd1306_clear();
    return ssd1306_update();
}

void ssd1306_clear(void)
{
    memset(s_fb, 0x00, sizeof(s_fb));
}

void ssd1306_set_pixel(int x, int y, bool on)
{
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) {
        return;
    }
    int page = y / 8;
    int bit  = y % 8;
    int idx  = page * SSD1306_WIDTH + x;
    if (on) {
        s_fb[idx] |= (1 << bit);
    } else {
        s_fb[idx] &= ~(1 << bit);
    }
}

void ssd1306_draw_hline(int x0, int x1, int y, bool on)
{
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    for (int x = x0; x <= x1; x++) {
        ssd1306_set_pixel(x, y, on);
    }
}

void ssd1306_draw_vline(int x, int y0, int y1, bool on)
{
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    for (int y = y0; y <= y1; y++) {
        ssd1306_set_pixel(x, y, on);
    }
}

void ssd1306_draw_vline_dashed(int x, int y0, int y1)
{
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    for (int y = y0; y <= y1; y++) {
        if ((y % 3) == 0) {
            ssd1306_set_pixel(x, y, true);
        }
    }
}

void ssd1306_draw_char(int x, int y, char c)
{
    for (size_t i = 0; i < FONT5X7_COUNT; i++) {
        if (font5x7_table[i].c == c) {
            for (int col = 0; col < 5; col++) {
                uint8_t colbits = font5x7_table[i].col[col];
                for (int row = 0; row < 7; row++) {
                    bool on = (colbits >> row) & 0x01;
                    ssd1306_set_pixel(x + col, y + row, on);
                }
            }
            return;
        }
    }
    // Caracter no soportado -> se deja en blanco (no interrumpe el texto).
}

void ssd1306_draw_string(int x, int y, const char *s)
{
    int cx = x;
    while (*s) {
        ssd1306_draw_char(cx, y, *s);
        cx += 6; // 5 px de glifo + 1 px de espacio
        s++;
    }
}

esp_err_t ssd1306_update(void)
{
    esp_err_t err;

    err = ssd1306_cmd(0x21); // Column address
    if (err != ESP_OK) return err;
    err = ssd1306_cmd(0);
    if (err != ESP_OK) return err;
    err = ssd1306_cmd(SSD1306_WIDTH - 1);
    if (err != ESP_OK) return err;

    err = ssd1306_cmd(0x22); // Page address
    if (err != ESP_OK) return err;
    err = ssd1306_cmd(0);
    if (err != ESP_OK) return err;
    err = ssd1306_cmd((SSD1306_HEIGHT / 8) - 1);
    if (err != ESP_OK) return err;

    // Se envia el framebuffer en bloques con el byte de control 0x40 (datos).
    uint8_t chunk[65];
    chunk[0] = 0x40;
    size_t total = sizeof(s_fb);
    size_t sent = 0;
    while (sent < total) {
        size_t n = total - sent;
        if (n > 64) n = 64;
        memcpy(&chunk[1], &s_fb[sent], n);
        err = i2c_master_write_to_device(s_port, SSD1306_ADDR, chunk, n + 1,
                                          pdMS_TO_TICKS(100));
        if (err != ESP_OK) return err;
        sent += n;
    }
    return ESP_OK;
}
