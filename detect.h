/**
 * @file    detect.h
 * @brief   Dual MQ-series gas sensor live monitor via STM32F446RE ADC.
 *          Reads two resistor-divider-scaled sensor outputs (PA0/PA1),
 *          reports both the ADC-pin voltage and the reconstructed
 *          sensor-side (AOUT) voltage.
 *
 * @note    Simpler sibling of the piezo detect module — no RMS
 *          batching, baseline, or SPC rules here, since the goal is
 *          a live readout while sensors are physically moved between
 *          open air and the fermentation container.
 *          Follows Barr Group Embedded C Coding Standard.
 */

#ifndef DETECT_H
#define DETECT_H

#include <stdint.h>

/*
 * -- ADC Channel Assignment --------------------------------------------------
 *
 *  CH0 (PA0) : Sensor 1 divider node  (e.g. MQ-3 alcohol)
 *  CH1 (PA1) : Sensor 2 divider node  (e.g. MQ-135 CO2 / air quality)
 */
#define SENSOR1_ADC_CHANNEL     (0U)
#define SENSOR2_ADC_CHANNEL     (1U)

/*
 * -- Voltage Divider ----------------------------------------------------------
 *
 *  Each sensor's 5V AOUT is scaled down through a two-resistor divider
 *  before reaching the STM32 ADC pin (VDDA max ~3.3V):
 *
 *      AOUT --[R1]-- node --[R2]-- GND
 *                     |
 *                    ADC pin
 *
 *  V_node = V_AOUT * (R2 / (R1 + R2))
 *
 *  Reversing this lets us report the sensor's actual AOUT voltage
 *  for reference, not just the scaled-down ADC pin voltage.
 */
#define DIVIDER_R1_OHMS         (10000.0f)   /* AOUT side   */
#define DIVIDER_R2_OHMS         (22000.0f)   /* GND side    */
#define DIVIDER_RATIO           (DIVIDER_R2_OHMS \
                                  / (DIVIDER_R1_OHMS + DIVIDER_R2_OHMS))

#define SAMPLE_DELAY_MS         (250U)

/*
 * -- Data Types -----------------------------------------------------------
 */

typedef struct
{
    uint16_t adc_raw;       /* raw 12-bit ADC code                  */
    float    adc_volts;     /* voltage actually seen at the ADC pin */
    float    aout_volts;    /* reconstructed sensor AOUT voltage    */
} sensor_reading_t;

typedef struct
{
    uint32_t          sample_num;
    sensor_reading_t  sensor1;
    sensor_reading_t  sensor2;
} reading_t;

/*
 * -- Function Prototypes ----------------------------------------------------
 */

/**
 * @brief  Read one sensor channel and compute both divider-side and
 *         sensor-side voltages.
 */
sensor_reading_t sensor_read(uint8_t adc_channel);

/**
 * @brief  Read both sensors once. Intended to be called in a tight
 *         loop from main() for a live readout.
 */
reading_t detect_take_reading(uint32_t sample_num);

#endif /* DETECT_H */