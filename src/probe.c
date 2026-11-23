#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "probe.h"
#include "calc.h"

#define BUZZER_GPIO 0
#define BUZZER_IDLE_LEVEL 1 // 1 = active-low
#define FOOTSWITCH_GPIO 17
#define FOOTSWITCH_DETECT_GPIO 15  // high = plug inserted (R35 pull-up)
#define FOOTSWITCH_PRESSED_LEVEL 0 // 0 = NO pedal, 1 = NC pedal
#define PROBE_KILL_GPIO 20

#define I2C_PORT i2c0
#define MCP4726_ADDR 0x62

static void mcp4726_set_voltage(uint16_t value);

static volatile alarm_id_t probe_alarm = 0;
static volatile alarm_id_t buzzer_alarm = 0;
static volatile bool alarm_running = false;
static volatile uint32_t alarm_started_us = 0;
static volatile uint32_t alarm_duration_us = 0;
static volatile bool trigger_dac_output = false;

static volatile bool output_active = false;
static volatile bool stop_dac_output = false;

static bool footswitch_is_pressed = false;
static bool footswitch_was_pressed = false;

static bool footswitch_armed = false;
static uint32_t footswitch_released_since_ms = 0;

static uint16_t *adc_values;

static bool footswitch_pressed_now(void)
{
    return footswitch_armed &&
           gpio_get(FOOTSWITCH_DETECT_GPIO) &&
           gpio_get(FOOTSWITCH_GPIO) == FOOTSWITCH_PRESSED_LEVEL;
}

static void buzzer_set(bool on)
{
    gpio_put(BUZZER_GPIO, on ? !BUZZER_IDLE_LEVEL : BUZZER_IDLE_LEVEL);
}

void probe_init(uint16_t *adc)
{
    adc_values = adc;

    gpio_init(PROBE_KILL_GPIO);
    gpio_put(PROBE_KILL_GPIO, 1); // killed by default
    gpio_set_dir(PROBE_KILL_GPIO, GPIO_OUT);

    gpio_init(BUZZER_GPIO);
    gpio_put(BUZZER_GPIO, BUZZER_IDLE_LEVEL);
    gpio_set_dir(BUZZER_GPIO, GPIO_OUT);

    gpio_init(FOOTSWITCH_GPIO);
    gpio_set_dir(FOOTSWITCH_GPIO, GPIO_IN);
    gpio_pull_up(FOOTSWITCH_GPIO);

    gpio_init(FOOTSWITCH_DETECT_GPIO);
    gpio_set_dir(FOOTSWITCH_DETECT_GPIO, GPIO_IN);
    gpio_disable_pulls(FOOTSWITCH_DETECT_GPIO);

    mcp4726_set_voltage(0);
}

static int64_t buzzer_off_callback(alarm_id_t id, void *user_data)
{
    (void)id;
    (void)user_data;

    buzzer_set(false);
    buzzer_alarm = 0;

    return 0;
}

static void buzzer_beep(uint32_t duration_ms)
{
    if (buzzer_alarm != 0)
    {
        cancel_alarm(buzzer_alarm);
        buzzer_alarm = 0;
    }

    buzzer_set(true);

    buzzer_alarm = add_alarm_in_ms(
        duration_ms,
        buzzer_off_callback,
        NULL,
        false);
}

static void mcp4726_set_voltage(uint16_t value)
{
    if (value > 4095)
        value = 4095;

    uint8_t buf[3];
    buf[0] = 0x40;
    buf[1] = (value >> 4) & 0xFF;
    buf[2] = (value & 0x0F) << 4;

    i2c_write_blocking(
        I2C_PORT,
        MCP4726_ADDR,
        buf,
        3,
        false);
}

static int64_t probe_timer_callback(alarm_id_t id, void *user_data)
{
    (void)id;
    (void)user_data;

    probe_alarm = 0;
    trigger_dac_output = false;
    alarm_running = false;
    stop_dac_output = true;

    return 0;
}

void probe_update(void)
{
    uint32_t now = to_ms_since_boot(get_absolute_time());
    bool plug_in = gpio_get(FOOTSWITCH_DETECT_GPIO);
    bool raw_pressed = plug_in && gpio_get(FOOTSWITCH_GPIO) == FOOTSWITCH_PRESSED_LEVEL;

    if (!plug_in)
    {
        footswitch_armed = false; // re-arm on every plug insertion
        footswitch_released_since_ms = now;
    }
    else if (raw_pressed)
    {
        footswitch_released_since_ms = now;
    }
    else if (!footswitch_armed && now - footswitch_released_since_ms >= 50)
    {
        footswitch_armed = true;
    }

    footswitch_is_pressed = footswitch_pressed_now();

    // Footswitch newly pressed -> start pulse for configured duration (e.g. 1s)
    if (footswitch_is_pressed && !footswitch_was_pressed)
    {
        if (probe_alarm == 0 && !output_active)
        {
            int64_t pulse_ms = calc_delay_ms(adc_values[0]);

            if (pulse_ms < 10)
                pulse_ms = 10;
            stop_dac_output = false;
            output_active = true;
            alarm_started_us = time_us_32();
            alarm_duration_us = (uint32_t)(pulse_ms * 1000LL);
            alarm_running = true;

            gpio_put(PROBE_KILL_GPIO, 0); // allow output during active pulse
            mcp4726_set_voltage(calc_dac_value(adc_values[1]));
            buzzer_beep(300);

            probe_alarm = add_alarm_in_ms(
                pulse_ms,
                probe_timer_callback,
                NULL,
                false);
        }
    }

    // Pulse duration elapsed -> cut output even if pedal is still held
    if (stop_dac_output)
    {
        stop_dac_output = false;
        output_active = false;

        gpio_put(PROBE_KILL_GPIO, 1); // hardware kill first
        mcp4726_set_voltage(0);
    }

    // Footswitch released -> cancel any remaining pulse and ensure output is off
    if (!footswitch_is_pressed && footswitch_was_pressed)
    {
        if (probe_alarm != 0)
        {
            cancel_alarm(probe_alarm);
            probe_alarm = 0;
        }

        if (buzzer_alarm != 0)
        {
            cancel_alarm(buzzer_alarm);
            buzzer_alarm = 0;
        }

        stop_dac_output = false;
        output_active = false;
        buzzer_set(false);
        gpio_put(PROBE_KILL_GPIO, 1); // hardware kill first
        mcp4726_set_voltage(0);
        alarm_running = false;
    }
    footswitch_was_pressed = footswitch_is_pressed;
}

uint32_t probe_alarm_elapsed_cs(void)
{
    if (!alarm_running)
        return 0;

    uint32_t elapsed_us = time_us_32() - alarm_started_us;
    if (elapsed_us >= alarm_duration_us)
        elapsed_us = alarm_duration_us;

    return elapsed_us / 10000;
}
