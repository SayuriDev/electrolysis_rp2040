#ifndef OLED_H
#define OLED_H

#include "u8g2.h"

void oled_init(uint16_t *adc, uint16_t *mux);
void oled_main_screen(void);

#endif