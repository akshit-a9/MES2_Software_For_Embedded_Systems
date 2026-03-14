#include "stm32f4xx.h"
#include "uart.h"
#include "delay.h"
#include <stdio.h>
#include <string.h>

#define NUM_PRIORITIES   8
#define TASKS_PER_PRIO    8
#define TOTAL_TASKS      (NUM_PRIORITIES * TASKS_PER_PRIO)   
#define SPORADIC_PRIO     7

volatile int sporadic_pending = 0;
volatile int sporadic_count = 0;
volatile int sporadic_next_exec = 1; 

/* Debounce: to avoid one press marking as two jobs entered sometimes */
#define DEBOUNCE_MS  400
volatile uint32_t last_press_tick = 0;

/*Integer-Decimal String*/
static void int_to_str(int val, char *buf)
{
    char tmp[12];
    int  i = 0;
    int  neg = 0;

    if (val < 0) { neg = 1; val = -val; }
    if (val == 0) { tmp[i++] = '0'; }
    while (val > 0) { tmp[i++] = '0' + (val % 10); val /= 10; }
    if (neg) tmp[i++] = '-';

    /* reverse into buf */
    {
        int j;
        for (j = 0; j < i; j++)
            buf[j] = tmp[i - 1 - j];
        buf[i] = '\0';
    }
}

/* ── EXTI0 initialisation (PA0, rising edge, with interrupt) ───────────────── */
static void EXTI0_Init(void)
{

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    SYSCFG->EXTICR[0] &= ~0xFU;

    EXTI->RTSR |= EXTI_RTSR_TR0;
    EXTI->FTSR &= ~EXTI_FTSR_TR0; 

    EXTI->IMR |= EXTI_IMR_MR0;

    NVIC_SetPriority(EXTI0_IRQn, 2);
    NVIC_EnableIRQ(EXTI0_IRQn);
}

/* Blue Button Pressed */
void EXTI0_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR0)
    {
        EXTI->PR = EXTI_PR_PR0;       

        /* Software debounce: ignore if pressed too soon after last press */
        if ((ticks - last_press_tick) < DEBOUNCE_MS)
            return;
        last_press_tick = ticks;

        sporadic_pending++;
        sporadic_count++;

        UART_SendString("Sporadic task ");
        {
            char num[4];
            int_to_str(sporadic_count, num);
            UART_SendString(num);
        }
        UART_SendString(" queued at prio ");
        {
            char num[4];
            int_to_str(SPORADIC_PRIO, num);
            UART_SendString(num);
        }
        UART_SendString("\r\n");
    }
}

/* Mian Loop */
static void run_task(int priority, int task_id)
{
    char line[64];
    char p[4], t[4];

    int_to_str(priority, p);
    int_to_str(task_id,  t);

    strcpy(line, "Prio ");
    strcat(line, p);
    strcat(line, " | task ");
    strcat(line, t);
    strcat(line, " running\r\n");

    UART_SendString(line);
    delay_ms(350);
}

/* For interrupts by sporadic tasks*/
static void run_sporadic(int priority)
{
    char line[64];
    char p[4];

    int_to_str(priority, p);

    {
        char id[4];
        int_to_str(sporadic_next_exec, id);

        strcpy(line, ">> [Sporadic] task ");
        strcat(line, id);
        strcat(line, " | Prio ");
        strcat(line, p);
        strcat(line, " running\r\n");
    }

    UART_SendString(line);
    delay_ms(350);

    sporadic_next_exec++;
    sporadic_pending--;
}

int main(void)
{
    int prio, t;

    SystemInit();
    SysTick_Init();
    UART2_Init();
    EXTI0_Init();

    while (1)
    {
        for (prio = 0; prio < NUM_PRIORITIES; prio++)
        {
            /* Run the 5 regular tasks for this priority level */
            for (t = 0; t < TASKS_PER_PRIO; t++)
            {
                run_task(prio, prio * TASKS_PER_PRIO + t);
            }

            while (sporadic_pending > 0 && SPORADIC_PRIO == prio)
            {
                run_sporadic(prio);
            }
        }
    }
}
