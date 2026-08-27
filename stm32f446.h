/**
 * @file    stm32f446.h
 * @brief   Minimal register definitions for STM32F446RE.
 *          Only what we need: RCC, GPIOA, ADC1, USART2, SysTick.
 *
 * @note    Barr Group style. Unchanged from prior project.
 */

#ifndef STM32F446_H
#define STM32F446_H

#include <stdint.h>

/* ── Base Addresses ───────────────────────────────────────────── */

#define PERIPH_BASE         (0x40000000UL)
#define AHB1_BASE           (PERIPH_BASE + 0x00020000UL)
#define APB1_BASE           (PERIPH_BASE + 0x00000000UL)
#define APB2_BASE           (PERIPH_BASE + 0x00010000UL)

#define GPIOA_BASE          (AHB1_BASE  + 0x0000UL)
#define RCC_BASE            (AHB1_BASE  + 0x3800UL)
#define ADC1_BASE           (APB2_BASE  + 0x2000UL)
#define USART2_BASE         (APB1_BASE  + 0x4400UL)
#define SYSTICK_BASE        (0xE000E010UL)

/* ── Register Access Macro ────────────────────────────────────── */

#define REG(addr)  (*(volatile uint32_t *)(addr))

/* ── RCC ──────────────────────────────────────────────────────── */

#define RCC_AHB1ENR         REG(RCC_BASE + 0x30U)
#define RCC_APB1ENR         REG(RCC_BASE + 0x40U)
#define RCC_APB2ENR         REG(RCC_BASE + 0x44U)

/* ── GPIOA ────────────────────────────────────────────────────── */

#define GPIOA_MODER         REG(GPIOA_BASE + 0x00U)
#define GPIOA_ODR           REG(GPIOA_BASE + 0x14U)
#define GPIOA_BSRR          REG(GPIOA_BASE + 0x18U)
#define GPIOA_AFRL          REG(GPIOA_BASE + 0x20U)

/* ── ADC1 ─────────────────────────────────────────────────────── */

#define ADC1_SR             REG(ADC1_BASE + 0x00U)
#define ADC1_CR1            REG(ADC1_BASE + 0x04U)
#define ADC1_CR2            REG(ADC1_BASE + 0x08U)
#define ADC1_SMPR2          REG(ADC1_BASE + 0x10U)
#define ADC1_SQR1           REG(ADC1_BASE + 0x2CU)
#define ADC1_SQR3           REG(ADC1_BASE + 0x34U)
#define ADC1_DR             REG(ADC1_BASE + 0x4CU)

/* ── USART2 ───────────────────────────────────────────────────── */

#define USART2_SR           REG(USART2_BASE + 0x00U)
#define USART2_DR           REG(USART2_BASE + 0x04U)
#define USART2_BRR          REG(USART2_BASE + 0x08U)
#define USART2_CR1          REG(USART2_BASE + 0x0CU)

/* ── SysTick ──────────────────────────────────────────────────── */

#define STK_CTRL            REG(SYSTICK_BASE + 0x00U)
#define STK_LOAD            REG(SYSTICK_BASE + 0x04U)
#define STK_VAL             REG(SYSTICK_BASE + 0x08U)

#endif /* STM32F446_H */