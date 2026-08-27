/**
 * @file    system.c
 * @brief   Hardware init for STM32F446RE Nucleo-64.
 *
 *          Clock:   HSI 16 MHz (no PLL — keeps it simple)
 *          UART:    USART2 @ 115200 on PA2 (TX via ST-LINK VCP)
 *          ADC:     ADC1, channel-selectable — PA0 (ch0) + PA1 (ch1)
 *          LED:     LD2 on PA5
 *          SysTick: 1 ms interrupt for delay_ms()
 *
 * @note    Extended from prior (piezo) project: that version only
 *          ever read ADC1 channel 0. This one adds PA1 as a second
 *          analog input and switches to a channel-select read
 *          function so both gas sensors can share ADC1.
 */

#include "system.h"
#include "stm32f446.h"

#include <stdint.h>
#include <sys/stat.h>

/* ═══════════════════════════════════════════════════════════════
 *  SysTick — 1 ms tick
 * ═══════════════════════════════════════════════════════════════ */

static volatile uint32_t g_ms_ticks = 0U;

void SysTick_Handler(void)
{
    g_ms_ticks++;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = g_ms_ticks;

    while ((g_ms_ticks - start) < ms)
    {
        /* spin */
    }
}

/* ═══════════════════════════════════════════════════════════════
 *  UART2 — printf redirect
 * ═══════════════════════════════════════════════════════════════ */

static void uart2_putchar(char c)
{
    while (!(USART2_SR & (1U << 7)))   /* Wait for TXE */
    {
    }

    USART2_DR = (uint32_t)c;
}

/**
 * @brief  Override newlib _write so printf() goes to UART2.
 */
int _write(int file, char * p_buf, int len)
{
    (void)file;

    for (int i = 0; i < len; i++)
    {
        if (p_buf[i] == '\n')
        {
            uart2_putchar('\r');
        }

        uart2_putchar(p_buf[i]);
    }

    return len;
}

/* ═══════════════════════════════════════════════════════════════
 *  Newlib stubs (heap, file I/O placeholders)
 * ═══════════════════════════════════════════════════════════════ */

extern uint32_t _ebss;

void * _sbrk(int incr)
{
    static uint8_t * p_heap = 0;

    if (p_heap == 0)
    {
        p_heap = (uint8_t *)&_ebss;
    }

    uint8_t * p_prev = p_heap;
    p_heap += incr;

    return p_prev;
}

int _close(int fd)                           { (void)fd; return -1; }
int _lseek(int fd, int ptr, int dir)         { (void)fd; (void)ptr; (void)dir; return 0; }
int _read(int fd, char * p, int len)         { (void)fd; (void)p; (void)len; return 0; }
int _fstat(int fd, struct stat * st)         { (void)fd; st->st_mode = S_IFCHR; return 0; }
int _isatty(int fd)                          { (void)fd; return 1; }
int _getpid(void)                            { return 1; }
int _kill(int pid, int sig)                  { (void)pid; (void)sig; return -1; }

/* ═══════════════════════════════════════════════════════════════
 *  ADC1 — Channel-selectable read (PA0 = ch0, PA1 = ch1)
 * ═══════════════════════════════════════════════════════════════ */

#define ADC_VOLTS_PER_BIT   (3.3f / 4096.0f)

uint16_t adc_read_channel(uint8_t channel)
{
    /* SQR3 low 5 bits select which channel converts next */
    ADC1_SQR3 = (uint32_t)(channel & 0x1FU);

    ADC1_CR2 |= (1U << 30);                /* SWSTART: begin conversion  */

    while (!(ADC1_SR & (1U << 1)))          /* Wait for EOC              */
    {
    }

    return (uint16_t)(ADC1_DR & 0xFFFU);    /* 12-bit result             */
}

float adc_to_volts(uint16_t raw)
{
    return (float)raw * ADC_VOLTS_PER_BIT;
}

/* ═══════════════════════════════════════════════════════════════
 *  LED LD2 — PA5
 * ═══════════════════════════════════════════════════════════════ */

void led_on(void)     { GPIOA_BSRR = (1U << 5);  }
void led_off(void)    { GPIOA_BSRR = (1U << 21); }
void led_toggle(void) { GPIOA_ODR ^= (1U << 5);  }

/* ═══════════════════════════════════════════════════════════════
 *  System Init — call once at top of main()
 * ═══════════════════════════════════════════════════════════════ */

void system_init(void)
{
    /*
     * Enable peripheral clocks
     *   AHB1: GPIOA  (bit 0)
     *   APB1: USART2 (bit 17)
     *   APB2: ADC1   (bit 8)
     */
    RCC_AHB1ENR |= (1U << 0);
    RCC_APB1ENR |= (1U << 17);
    RCC_APB2ENR |= (1U << 8);

    /*
     * GPIO pin modes
     *   PA0: analog       (MODER = 11)  — ADC ch0, sensor 1 divider
     *   PA1: analog       (MODER = 11)  — ADC ch1, sensor 2 divider
     *   PA2: alt function (MODER = 10)  — USART2 TX
     *   PA5: output       (MODER = 01)  — LED LD2
     */
    GPIOA_MODER |=  (3U << 0);           /* PA0 = analog              */
    GPIOA_MODER |=  (3U << 2);           /* PA1 = analog              */

    GPIOA_MODER &= ~(3U << 4);
    GPIOA_MODER |=  (2U << 4);           /* PA2 = alt function        */
    GPIOA_AFRL  &= ~(0xFU << 8);
    GPIOA_AFRL  |=  (7U << 8);           /* PA2 AF7 = USART2_TX       */

    GPIOA_MODER &= ~(3U << 10);
    GPIOA_MODER |=  (1U << 10);          /* PA5 = output              */

    /*
     * USART2: 115200 baud, 8N1
     *   BRR = 16 MHz / 115200 ~= 0x8B
     */
    USART2_BRR = 0x008BU;
    USART2_CR1 = (1U << 13)              /* UE: USART enable          */
               | (1U << 3);              /* TE: transmit enable       */

    /*
     * ADC1: 12-bit, single conversion, channel selected per-call
     *   Sample time = 480 cycles on ch0 AND ch1 (SMP = 111b each)
     *   MQ-sensor divider output is relatively high impedance,
     *   so a long sample time gives a more accurate conversion
     *   than the 84-cycle setting used for the low-impedance
     *   piezo amp output in the prior project.
     *   Sequence length = 1 conversion (SQR1 L = 0)
     */
    ADC1_CR1   = 0U;                     /* 12-bit resolution (RES=0) */
    ADC1_SMPR2 = (7U << 0)               /* 480 cycles on ch0         */
               | (7U << 3);              /* 480 cycles on ch1         */
    ADC1_SQR1  = 0U;                     /* 1 conversion in sequence  */
    ADC1_SQR3  = 0U;                     /* channel selected at read  */
    ADC1_CR2   = (1U << 0);              /* ADON: power on ADC        */

    /*
     * SysTick: 1 ms tick @ 16 MHz HSI
     *   LOAD = 16000 - 1
     */
    STK_LOAD = 16000U - 1U;
    STK_VAL  = 0U;
    STK_CTRL = (1U << 2)                 /* CLKSOURCE: processor clk  */
             | (1U << 1)                 /* TICKINT: enable interrupt  */
             | (1U << 0);                /* ENABLE                    */
}