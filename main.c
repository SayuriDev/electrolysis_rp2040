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

    uint pwm_slice_num = pwm_gpio_to_slice_num(charge_pump_1_gpio);
    pwm_set_clkdiv(pwm_slice_num, 125.0f);
    pwm_set_wrap(pwm_slice_num, 999);

    pwm_set_output_invert(pwm_slice_num, false, true); // invert channel B
    
    pwm_set_chan_level(pwm_slice_num, PWM_CHAN_A, 500);
    pwm_set_chan_level(pwm_slice_num, PWM_CHAN_B, 500);

    pwm_set_enabled(pwm_slice_num, true);

    while (true) {
        tight_loop_contents();
    }
}