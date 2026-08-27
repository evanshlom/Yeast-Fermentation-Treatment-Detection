/**
 * @file    system.h
 * @brief   Hardware init + helpers for STM32F446RE Nucleo.
 *
 * @note    Extended from prior project: adc_read() replaced with
 *          adc_read_channel() so both PA0 (ch0) and PA1 (ch1) can
 *          be sampled from the same ADC1 peripheral.
 */

#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>

void     system_init(void);
void     delay_ms(uint32_t ms);
uint16_t adc_read_channel(uint8_t channel);
float    adc_to_volts(uint16_t raw);
void     led_on(void);
void     led_off(void);
void     led_toggle(void);

#endif /* SYSTEM_H */