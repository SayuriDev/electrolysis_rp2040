#include "pico/stdlib.h"

int main() {
    const uint buzzer_gpio = 2;

    gpio_init(buzzer_gpio);
    gpio_set_dir(buzzer_gpio, GPIO_OUT);


    while (true) {
        gpio_put(buzzer_gpio, 1);
        sleep_ms(400);
        gpio_put(buzzer_gpio, 0);
        sleep_ms(5000);
    }
}