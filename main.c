#include "pico/stdlib.h"
#include "hardware/pwm.h"

int main() {
    const uint buzzer_gpio = 0;
    gpio_init(buzzer_gpio);
    gpio_set_dir(buzzer_gpio, GPIO_OUT);

    const uint charge_pump_1_gpio = 2; // slice 1, channel A
    gpio_set_function(charge_pump_1_gpio, GPIO_FUNC_PWM);

    const uint charge_pump_2_gpio = 3; // slice 1, channel B
    gpio_set_function(charge_pump_2_gpio, GPIO_FUNC_PWM);

    uint charge_pump_pwm_slice = pwm_gpio_to_slice_num(charge_pump_1_gpio);
    pwm_set_clkdiv(charge_pump_pwm_slice, 125.0f);
    pwm_set_wrap(charge_pump_pwm_slice, 999);

    pwm_set_output_polarity(charge_pump_pwm_slice, false, true); // invert channel B
    
    pwm_set_chan_level(charge_pump_pwm_slice, PWM_CHAN_A, 500);
    pwm_set_chan_level(charge_pump_pwm_slice, PWM_CHAN_B, 500);

    pwm_set_enabled(charge_pump_pwm_slice, true);

    while (true) {
        tight_loop_contents();
    }
}