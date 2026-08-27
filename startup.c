/**
 * @file    startup.c
 * @brief   Vector table + reset handler for STM32F446RE.
 *          Copies .data from flash -> RAM, zeros .bss, enables FPU.
 *
 * @note    Unchanged from prior project.
 */

#include <stdint.h>

/* Symbols from linker script */
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _estack;

/* Forward declarations */
extern int main(void);
void Reset_Handler(void);
void Default_Handler(void);
void SysTick_Handler(void);

/* ── Vector Table ─────────────────────────────────────────────── */

__attribute__((section(".isr_vector")))
void (* const g_vectors[])(void) =
{
    (void (*)(void))&_estack,   /*  0: Initial stack pointer     */
    Reset_Handler,              /*  1: Reset                     */
    Default_Handler,            /*  2: NMI                       */
    Default_Handler,            /*  3: HardFault                 */
    Default_Handler,            /*  4: MemManage                 */
    Default_Handler,            /*  5: BusFault                  */
    Default_Handler,            /*  6: UsageFault                */
    0, 0, 0, 0,                 /*  7-10: Reserved               */
    Default_Handler,            /* 11: SVCall                    */
    Default_Handler,            /* 12: DebugMon                  */
    0,                          /* 13: Reserved                  */
    Default_Handler,            /* 14: PendSV                    */
    SysTick_Handler,            /* 15: SysTick                   */
};

/* ── Reset Handler ────────────────────────────────────────────── */

void Reset_Handler(void)
{
    /* Enable FPU: CP10 + CP11 full access */
    *(volatile uint32_t *)0xE000ED88UL |= (0xFUL << 20);

    /* Copy .data from flash to SRAM */
    uint32_t * p_src = &_sidata;
    uint32_t * p_dst = &_sdata;

    while (p_dst < &_edata)
    {
        *p_dst++ = *p_src++;
    }

    /* Zero .bss */
    p_dst = &_sbss;

    while (p_dst < &_ebss)
    {
        *p_dst++ = 0U;
    }

    main();

    /* Trap if main returns */
    while (1) { }
}

/* ── Default Handler (infinite loop) ─────────────────────────── */

void Default_Handler(void)
{
    while (1) { }
}