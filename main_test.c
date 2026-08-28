/**
 * @file    main_test.c
 * @brief   Entry point for live dual gas sensor monitor — TEST MODE.
 *          STM32F446RE Nucleo bare-metal.
 *
 * @note    Functionally identical to main.c (same sensor reading,
 *          same table output format). Kept as a separate file/binary
 *          so training-data collection (main.c) and model-testing
 *          sessions (this file) never require touching or reflashing
 *          each other's build. Only the banner text differs, purely
 *          so the terminal makes clear which mode is running.
 *          Follows Barr Group Embedded C Coding Standard.
 */

#include "detect.h"
#include "system.h"
#include "ml_model.h"

#include <stdio.h>

#define HEADER_REPEAT_INTERVAL   (40U)

static void print_startup_banner(void)
{
    printf("============================================================\n");
    printf("  Dual Gas Sensor Live Monitor -- TEST MODE\n");
    printf("  (feed this output into classify_live.py)\n");
    printf("  CO2  = PA0/A0 (MQ-135 air quality sensor)\n");
    printf("  ALC  = PA1/A1 (MQ-3-style alcohol sensor)\n");
    printf("  STM32F446RE ADC, 12-bit, 3.3V reference\n");
    printf("============================================================\n\n");

    printf("  Config:\n");
    printf("    Divider: %.0f ohm / %.0f ohm (ratio %.4f)\n",
           DIVIDER_R1_OHMS, DIVIDER_R2_OHMS, DIVIDER_RATIO);
    printf("    Sample interval: %lu ms\n\n",
           (unsigned long)SAMPLE_DELAY_MS);

    printf("  Notes:\n");
    printf("    Allow 1-2 min sensor warm-up before trusting readings.\n");
    printf("    Reset button or unplug to stop.\n\n");
}

static void print_table_header(void)
{
    printf("\n");
    printf("%-8s | %-8s %-8s %-8s | %-8s %-8s %-8s | %-18s\n",
           "Sample", "CO2raw", "CO2_ADCv", "CO2_AOUTv",
                     "ALCraw", "ALC_ADCv", "ALC_AOUTv",
           "Class");
    printf("---------|-------------------------|-------------------------|-------------------\n");
}

int main(void)
{
    system_init();

    print_startup_banner();

    uint32_t sample_num = 0U;

    while (1)
    {
        reading_t r = detect_take_reading(sample_num);

        int predicted = ml_predict_class(r.sensor1.aout_volts, r.sensor2.aout_volts);

        printf("%-8lu | %-8u %-8.3f %-8.3f | %-8u %-8.3f %-8.3f | %-18s\n",
               (unsigned long)sample_num,
               r.sensor1.adc_raw,
               r.sensor1.adc_volts,
               r.sensor1.aout_volts,
               r.sensor2.adc_raw,
               r.sensor2.adc_volts,
               r.sensor2.aout_volts,
               ML_CLASS_LABELS[predicted]);

        if ((sample_num % HEADER_REPEAT_INTERVAL) == 0U)
        {
            print_table_header();
        }

        sample_num++;

        delay_ms(SAMPLE_DELAY_MS);
    }
}