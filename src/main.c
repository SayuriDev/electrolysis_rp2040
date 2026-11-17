#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"

#include "oled.h"
#include "probe.h"

#define CHARGE_PUMP_1_GPIO 2 // slice 1, channel A
#define CHARGE_PUMP_2_GPIO 3 // slice 1, channel B
#define USER_BUTTON_GPIO 18
#define SDA_GPIO 4
#define SCL_GPIO 5

#define MUX_S0_GPIO 10
#define MUX_S1_GPIO 11
#define MUX_S2_GPIO 12

#define ADC_POTENTIOMETER_1_GPIO 26
#define ADC_POTENTIOMETER_2_GPIO 27
#define ADC_PROBE_GPIO 28
#define ADC_MUX_GPIO 29

#define I2C_PORT i2c0
#define I2C_BAUDRATE (400 * 1000) // fast mode (400kHz)

static struct repeating_timer adc_timer;
static struct repeating_timer oled_timer;

static uint16_t adc[3];
static uint16_t mux[3];
static float adc_filtered_f[3] = {0};
static float mux_filtered_f[3] = {0};

static void mux_init(void)
{
    gpio_init(MUX_S0_GPIO);
    gpio_set_dir(MUX_S0_GPIO, GPIO_OUT);

    gpio_init(MUX_S1_GPIO);
    gpio_set_dir(MUX_S1_GPIO, GPIO_OUT);

    gpio_init(MUX_S2_GPIO);
    gpio_set_dir(MUX_S2_GPIO, GPIO_OUT);
}

static void mux_select(uint8_t channel)
{ // channel range: 0-7 (A0-A7 pins)
    gpio_put(MUX_S0_GPIO, channel & 1);
    gpio_put(MUX_S1_GPIO, (channel >> 1) & 1);
    gpio_put(MUX_S2_GPIO, (channel >> 2) & 1);
}

/*
 *   adc[0] -> potentiometer_1
 *   adc[1] -> potentiometer_2
 *   adc[2] -> probe
 *
 *   mux[0] -> battery_voltage
 *   mux[1] -> battery_charging
 *   mux[2] -> positive probe
 */
static void read_adc_values(uint16_t adc[3], uint16_t mux[3])
{
    const float alpha = 0.15f;

    // adc 0-2
    for (uint8_t i = 0; i < 3; i++)
    {
        adc_select_input(i);
        uint32_t sum = 0;

        // take 16 samples
        for (uint8_t s = 0; s < 16; s++)
        {
            sum += adc_read();
        }
        uint16_t raw_oversampled = (uint16_t)(sum >> 4); // average from 16 samples

        // EMA filter
        if (adc_filtered_f[i] == 0.0f)
        {
            adc_filtered_f[i] = (float)raw_oversampled;
        }
        else
        {
            adc_filtered_f[i] = (alpha * (float)raw_oversampled) + ((1.0f - alpha) * adc_filtered_f[i]);
        }

        adc[i] = (uint16_t)(adc_filtered_f[i] + 0.5f);
    }

    // mux 0-2
    adc_select_input(3);
    for (uint8_t i = 0; i < 3; i++)
    {
        mux_select(i);
        busy_wait_us(2);
        adc_read(); // dummy read

        uint32_t sum = 0;
        for (uint8_t s = 0; s < 16; s++)
        {
            sum += adc_read();
        }
        uint16_t raw_oversampled = (uint16_t)(sum >> 4);

        // EMA filter
        if (mux_filtered_f[i] == 0.0f)
        {
            mux_filtered_f[i] = (float)raw_oversampled;
        }
        else
        {
            mux_filtered_f[i] = (alpha * (float)raw_oversampled) + ((1.0f - alpha) * mux_filtered_f[i]);
        }

        mux[i] = (uint16_t)(mux_filtered_f[i] + 0.5f);
    }
}
static bool adc_timer_callback(struct repeating_timer *t)
{
    (void)t;
    read_adc_values(adc, mux);
    return true;
}

static bool oled_timer_callback(struct repeating_timer *t)
{
    (void)t;
    oled_main_screen();
    return true;
}

int main()
{
    adc_init();
    adc_gpio_init(ADC_POTENTIOMETER_1_GPIO);
    adc_gpio_init(ADC_POTENTIOMETER_2_GPIO);
    adc_gpio_init(ADC_PROBE_GPIO);
    adc_gpio_init(ADC_MUX_GPIO);
    mux_init();

    gpio_set_function(CHARGE_PUMP_1_GPIO, GPIO_FUNC_PWM);
    gpio_set_function(CHARGE_PUMP_2_GPIO, GPIO_FUNC_PWM);

    uint charge_pump_pwm_slice = pwm_gpio_to_slice_num(CHARGE_PUMP_1_GPIO);
    pwm_set_clkdiv(charge_pump_pwm_slice, 125.0f);
    pwm_set_wrap(charge_pump_pwm_slice, 999);

    pwm_set_output_polarity(charge_pump_pwm_slice, false, true); // invert channel B

    pwm_set_chan_level(charge_pump_pwm_slice, PWM_CHAN_A, 500);
    pwm_set_chan_level(charge_pump_pwm_slice, PWM_CHAN_B, 500);

    pwm_set_enabled(charge_pump_pwm_slice, true);

    // setup i2c
    i2c_init(I2C_PORT, I2C_BAUDRATE);
    gpio_set_function(SDA_GPIO, GPIO_FUNC_I2C);
    gpio_set_function(SCL_GPIO, GPIO_FUNC_I2C);

    gpio_init(USER_BUTTON_GPIO);
    gpio_set_dir(USER_BUTTON_GPIO, GPIO_IN);
    gpio_pull_up(USER_BUTTON_GPIO);

    oled_init(adc, mux);

    probe_init(adc);
    add_repeating_timer_us(50, adc_timer_callback, NULL, &adc_timer);
    add_repeating_timer_ms(100, oled_timer_callback, NULL, &oled_timer);
    // bool is_pressed = !gpio_get(USER_BUTTON_GPIO);

    while (true)
    {
        probe_update();
    }
}