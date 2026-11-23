#ifndef PROBE_H
#define PROBE_H

void probe_init(uint16_t *adc);
void probe_update(void);
uint32_t probe_alarm_elapsed_cs(void);

#endif