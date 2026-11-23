#include "calc.h"

#define CURRENT_MAX_UA 2000
#define TIME_MIN_CS 100  // 1.00 s
#define TIME_MAX_CS 1500 // 15.00 s

#define BAT_ADC_MIN 2640 // ~3.3 V (empty)
#define BAT_ADC_MAX 3410 // 4.2 V (full)

uint16_t calc_current_ua(uint16_t pot_raw)
{
    return ((uint32_t)pot_raw * CURRENT_MAX_UA) / 4095;
}

uint16_t calc_dac_value(uint16_t pot_raw)
{
    return (uint32_t)pot_raw * 866 / 1000;
}

uint32_t calc_time_cs(uint16_t pot_raw)
{
    uint32_t cs = ((uint32_t)pot_raw * TIME_MAX_CS) / 4095;
    if (cs < TIME_MIN_CS)
        cs = TIME_MIN_CS;
    if (cs > TIME_MAX_CS)
        cs = TIME_MAX_CS;
    return cs;
}

uint32_t calc_delay_ms(uint16_t pot_raw)
{
    return calc_time_cs(pot_raw) * 10;
}

uint8_t calc_battery_percent(uint16_t bat_raw)
{
    if (bat_raw <= BAT_ADC_MIN)
        return 0;
    if (bat_raw >= BAT_ADC_MAX)
        return 100;
    return ((uint32_t)(bat_raw - BAT_ADC_MIN) * 100) / (BAT_ADC_MAX - BAT_ADC_MIN);
}

float calc_probe_voltage(uint16_t mux_raw)
{
    return mux_raw * 7.29f;
}
