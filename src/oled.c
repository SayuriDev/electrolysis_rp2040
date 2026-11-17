#include "oled.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#define I2C_PORT i2c0

#define OLED_SDA 4
#define OLED_SCL 5

#define OLED_ADDR 0x3C

static uint16_t *adc_values;
static uint16_t *mux_values;

static uint8_t u8x8_byte_i2c(
    u8x8_t *u8x8,
    uint8_t msg,
    uint8_t arg_int,
    void *arg_ptr)
{
    static uint8_t buf[256];
    static uint16_t len;

    switch (msg)
    {
    case U8X8_MSG_BYTE_INIT:
        break;

    case U8X8_MSG_BYTE_START_TRANSFER:
        len = 0;
        break;

    case U8X8_MSG_BYTE_SEND:
        if (len + arg_int > sizeof(buf))
            return 0;
        memcpy(&buf[len], arg_ptr, arg_int);
        len += arg_int;
        break;

    case U8X8_MSG_BYTE_END_TRANSFER:
        if (i2c_write_blocking(I2C_PORT, u8x8_GetI2CAddress(u8x8),
                               buf, len, false) < 0)
            return 0;
        break;

    default:
        return 0;
    }

    return 1;
}

static uint8_t u8x8_gpio_and_delay_rp2040(
    u8x8_t *u8x8,
    uint8_t msg,
    uint8_t arg_int,
    void *arg_ptr)
{
    switch (msg)
    {
    case U8X8_MSG_DELAY_MILLI:
        sleep_ms(arg_int);
        break;

    case U8X8_MSG_DELAY_10MICRO:
        sleep_us(arg_int * 10);
        break;

    case U8X8_MSG_DELAY_NANO:
        sleep_us(1);
        break;

    case U8X8_MSG_GPIO_AND_DELAY_INIT:
        break;

    default:
        break;
    }

    return 1;
}

static const uint8_t charging_icon[] U8X8_PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x30, 0x00, 0x00, 0x38, 0x00, 0x00, 0x1c, 0x00,
    0x00, 0x1e, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x7f,
    0x00, 0x00, 0x3c, 0x00, 0x00, 0x1c, 0x00, 0x00,
    0x0e, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};
static u8g2_t u8g2;

void oled_init(uint16_t *adc, uint16_t *mux)
{
    adc_values = adc;
    mux_values = mux;
    /* already done in main.c
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL, GPIO_FUNC_I2C);

    gpio_pull_up(OLED_SDA);
    gpio_pull_up(OLED_SCL); */

    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        &u8g2,
        U8G2_R0,
        u8x8_byte_i2c,
        u8x8_gpio_and_delay_rp2040);

    u8g2_SetI2CAddress(&u8g2, OLED_ADDR);

    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
}

void oled_main_screen(void)
{
    char buf[20];

    u8g2_ClearBuffer(&u8g2);

    u8g2_SetFont(&u8g2, u8g2_font_helvB14_tr);

    uint16_t current_pot_raw = adc_values[1];
    uint16_t current_pot_scaled = ((uint32_t)current_pot_raw * 2000) / 4095; // 0–20, where 200 = 2.00 mA
    uint16_t ma_whole = current_pot_scaled / 1000;                           // the part before the decimal point
    uint16_t ma_frac = current_pot_scaled % 1000;                            // two digits after the decimal point
    snprintf(buf, sizeof(buf), "%u.%u mA", ma_whole, ma_frac);

    int str_width = u8g2_GetStrWidth(&u8g2, buf);
    int x = (128 - str_width) / 2;

    u8g2_DrawStr(&u8g2, x, 56, buf);

    float body_probe_voltage = mux_values[2] * 9.22f;
    // TODO: show this on screen

    // time potentiometer
    u8g2_SetFont(&u8g2, u8g2_font_helvB12_tr);

    uint16_t time_pot_raw = adc_values[0];
    uint32_t time_pot_scaled = ((uint32_t)time_pot_raw * 1500) / 4095; // 0–1500, where 1500 = 15.00s

    if (time_pot_scaled < 100)
        time_pot_scaled = 100; // min 1.00s
    if (time_pot_scaled > 1500)
        time_pot_scaled = 1500; // 15.00s clamp

    uint16_t secs_whole = time_pot_scaled / 100;
    uint16_t secs_frac = time_pot_scaled % 100;

    snprintf(buf, sizeof(buf), "%u.%us", secs_whole, secs_frac);

    u8g2_DrawStr(&u8g2, 5, 15, buf);

    // battery
    uint16_t bat_adc = mux_values[0];

    uint16_t bat_adc_min = 2640;
    uint16_t bat_adc_max = 3410;

    uint16_t bat_val;

    if (bat_adc <= bat_adc_min)
        bat_val = 0;
    else if (bat_adc >= bat_adc_max)
        bat_val = 100;
    else
    {
        bat_val = ((bat_adc - bat_adc_min) * 100) / (bat_adc_max - bat_adc_min);
    }

    u8g2_DrawRFrame(&u8g2, 105, 5, 20, 10, 2);

    // battery blinking when low
    static uint32_t blink_timer = 0;
    static bool blink_state = false;

    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (now - blink_timer >= 500)
    {
        blink_timer = now;
        blink_state = !blink_state;
    }

    if (bat_val >= 20 || blink_state)
    {
        if (bat_val <= 10) // avoid keeping width under 10
        {
            u8g2_DrawRBox(&u8g2, 105, 5, 10 / 5, 10, 1);
        }
        else
        {
            u8g2_DrawRBox(&u8g2, 105, 5, bat_val / 5, 10, 1);
        }
    }

    bool is_charging = mux_values[1] >= 100;

    if (is_charging)
    {
        u8g2_DrawXBMP(&u8g2, 80, 1, 23, 28, charging_icon);
    }

    u8g2_SendBuffer(&u8g2);
}