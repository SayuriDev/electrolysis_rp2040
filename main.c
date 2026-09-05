#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define BUZZER_GPIO 0
#define CHARGE_PUMP_1_GPIO 2  // slice 1, channel A
#define CHARGE_PUMP_2_GPIO 3  // slice 1, channel B

int main() {
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

    while (true) {
        tight_loop_contents();
    }
}