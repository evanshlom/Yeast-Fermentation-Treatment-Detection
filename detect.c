/**
 * @file    detect.c
 * @brief   Dual MQ-series gas sensor live monitor.
 *
 * @note    Follows Barr Group Embedded C Coding Standard.
 */

#include "detect.h"
#include "system.h"

sensor_reading_t sensor_read(uint8_t adc_channel)
{
    sensor_reading_t r;

    r.adc_raw    = adc_read_channel(adc_channel);
    r.adc_volts  = adc_to_volts(r.adc_raw);
    r.aout_volts = r.adc_volts / DIVIDER_RATIO;

    return r;
}

reading_t detect_take_reading(uint32_t sample_num)
{
    reading_t r;

    r.sample_num = sample_num;
    r.sensor1    = sensor_read(SENSOR1_ADC_CHANNEL);
    r.sensor2    = sensor_read(SENSOR2_ADC_CHANNEL);

    return r;
}