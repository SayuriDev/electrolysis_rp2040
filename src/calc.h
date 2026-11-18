#pragma once
#include <stdint.h>

// all inputs are raw 12-bit ADC values (0-4095)
uint16_t calc_current_ua(uint16_t pot_raw);
uint16_t calc_dac_value(uint16_t pot_raw);
uint32_t calc_time_cs(uint16_t pot_raw);        // delay in 1/100 s, 100-1500
uint32_t calc_delay_ms(uint16_t pot_raw);       // same delay in ms
uint8_t calc_battery_percent(uint16_t bat_raw); // 0-100
float calc_probe_voltage(uint16_t mux_raw);