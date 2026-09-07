#include "oled.h"

#include <stdbool.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#define I2C_PORT i2c0

#define OLED_SDA 4
#define OLED_SCL 5

#define OLED_ADDR 0x3C


static uint8_t u8x8_byte_i2c(
    u8x8_t *u8x8,
    uint8_t msg,
    uint8_t arg_int,
    void *arg_ptr)
{
    switch (msg)
    {
        case U8X8_MSG_BYTE_INIT:
            break;

        case U8X8_MSG_BYTE_START_TRANSFER:
            break;

        case U8X8_MSG_BYTE_SEND:
            i2c_write_blocking(
                I2C_PORT,
                u8x8_GetI2CAddress(u8x8),
                (uint8_t *)arg_ptr,
                arg_int,
                false
            );
            break;

        case U8X8_MSG_BYTE_END_TRANSFER:
            break;
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


static u8g2_t u8g2;


void oled_init(void)
{
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
        u8x8_gpio_and_delay_rp2040
    );

    u8g2_SetI2CAddress(&u8g2, OLED_ADDR << 1);

    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
}


void oled_main_screen(void)
{
    u8g2_ClearBuffer(&u8g2);
    TODO:
    u8g2_SendBuffer(&u8g2);
}