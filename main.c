#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"

#define BUZZER_GPIO 0
#define CHARGE_PUMP_1_GPIO 2  // slice 1, channel A
#define CHARGE_PUMP_2_GPIO 3  // slice 1, channel B
#define FOOTSWITCH_GPIO 17
#define USER_BUTTON_GPIO 18
#define SDA_GPIO 4
#define SCL_GPIO 5

#define MUX_S0_GPIO 12
#define MUX_S1_GPIO 11
#define MUX_S2_GPIO 10

#define ADC_POTENTIOMETER_1_GPIO 26
#define ADC_POTENTIOMETER_2_GPIO 27
#define ADC_PROBE_GPIO 28
#define ADC_MUX_GPIO 29

#define I2C_PORT i2c0
#define I2C_BAUDRATE (400 * 1000) // fast mode (400kHz)

static void mux_init(void) {
    gpio_init(MUX_S0_GPIO);
    gpio_set_dir(MUX_S0_GPIO, GPIO_OUT);

    gpio_init(MUX_S1_GPIO);
    gpio_set_dir(MUX_S1_GPIO, GPIO_OUT);

    gpio_init(MUX_S2_GPIO);
    gpio_set_dir(MUX_S2_GPIO, GPIO_OUT);
}

static void mux_select(uint8_t channel) { // channel range: 0-7 (A0-A7 pins)
    gpio_put(MUX_S0_GPIO, channel & 1);
    gpio_put(MUX_S1_GPIO, (channel >> 1) & 1);
    gpio_put(MUX_S2_GPIO, (channel >> 2) & 1);
}

int main() {
    adc_init();
    mux_init();
    gpio_init(BUZZER_GPIO);
    gpio_set_dir(BUZZER_GPIO, GPIO_OUT);

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

    gpio_set_pulls(SDA_GPIO, true, false);
    gpio_set_pulls(SCL_GPIO, true, false);

    gpio_init(FOOTSWITCH_GPIO);
    gpio_set_dir(FOOTSWITCH_GPIO, GPIO_IN);
    gpio_pull_up(FOOTSWITCH_GPIO);
    // bool is_pressed = !gpio_get(FOOTSWITCH_GPIO);

    gpio_init(USER_BUTTON_GPIO);
    gpio_set_dir(USER_BUTTON_GPIO, GPIO_IN);
    gpio_pull_up(USER_BUTTON_GPIO);
    // bool is_pressed = !gpio_get(USER_BUTTON_GPIO);


    while (true) {
        tight_loop_contents();
    }
}