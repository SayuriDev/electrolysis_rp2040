#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "probe.h"

#define BUZZER_GPIO 0
#define FOOTSWITCH_GPIO 17

#define I2C_PORT i2c0
#define MCP4726_ADDR 0x60

static alarm_id_t probe_alarm = 0;
static alarm_id_t buzzer_alarm = 0;

static bool footswitch_is_pressed = false;
static bool footswitch_was_pressed = false;

static uint16_t *adc_values;

void probe_init(uint16_t *adc) {
    adc_values = adc;

    gpio_init(BUZZER_GPIO);
    gpio_set_dir(BUZZER_GPIO, GPIO_OUT);

    gpio_init(FOOTSWITCH_GPIO);
    gpio_set_dir(FOOTSWITCH_GPIO, GPIO_IN);
    gpio_pull_up(FOOTSWITCH_GPIO);
}

static int64_t buzzer_off_callback(alarm_id_t id, void *user_data) {
    (void)id;
    (void)user_data;

    gpio_put(BUZZER_GPIO, 0);
    buzzer_alarm = 0;

    return 0;
}

static void buzzer_beep(uint32_t duration_ms) {
    if (buzzer_alarm != 0) {
        cancel_alarm(buzzer_alarm);
        buzzer_alarm = 0;
    }

    gpio_put(BUZZER_GPIO, 1);

    buzzer_alarm = add_alarm_in_ms(
        duration_ms,
        buzzer_off_callback,
        NULL,
        false
    );
}

static void mcp4726_set_voltage(uint16_t value) {
    if (value > 4095) value = 4095;

    uint8_t buf[3];

    buf[0] = 0x40;
    buf[1] = (value >> 4) & 0xFF;
    buf[2] = (value & 0x0F) << 4;

    i2c_write_blocking(
        I2C_PORT,
        MCP4726_ADDR,
        buf,
        3,
        false
    );
}

static int64_t probe_timer_callback(alarm_id_t id, void *user_data) {
    (void)id;
    (void)user_data;

    probe_alarm = 0;

    // footswitch released
    if (gpio_get(FOOTSWITCH_GPIO)) return 0;

    mcp4726_set_voltage(adc_values[1]);

    // check again in case footswitch was released
    if (gpio_get(FOOTSWITCH_GPIO)) {
        mcp4726_set_voltage(0);
        return 0;
    }

    buzzer_beep(300);

    return 0;
}

void probe_update(void) {
    footswitch_is_pressed = !gpio_get(FOOTSWITCH_GPIO);

    if (footswitch_is_pressed && !footswitch_was_pressed) {

        if (probe_alarm == 0) {

            float secs = adc_values[0] / 273.0f;
            int64_t delay_ms = (int64_t)(secs * 1000.0f);

            if (delay_ms < 10) delay_ms = 10;

            buzzer_beep(300);

            probe_alarm = add_alarm_in_ms(
                delay_ms,
                probe_timer_callback,
                NULL,
                false
            );
        }
    }

    // footswitch released
    if (!footswitch_is_pressed && footswitch_was_pressed) {

        if (probe_alarm != 0) {
            cancel_alarm(probe_alarm);
            probe_alarm = 0;
        }

        if (buzzer_alarm != 0) {
            cancel_alarm(buzzer_alarm);
            buzzer_alarm = 0;
        }

        gpio_put(BUZZER_GPIO, 0);
        mcp4726_set_voltage(0);
    }

    footswitch_was_pressed = footswitch_is_pressed;
}