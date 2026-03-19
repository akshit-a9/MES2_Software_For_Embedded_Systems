#include "stm32f4xx.h"
#include "uart.h"
#include "delay.h"
#include <stdio.h>
#include <string.h>

#define NUM_PRIORITIES   8
#define TASKS_PER_PRIO    8
#define TOTAL_TASKS      (NUM_PRIORITIES * TASKS_PER_PRIO)   
#define SPORADIC_PRIO_A   7   /* PA0 */
#define SPORADIC_PRIO_B   5   /* PA1 */
#define SPORADIC_PRIO_C   3   /* PA4 */

volatile int sporadic_pending_a = 0;
volatile int sporadic_count_a   = 0;
volatile int sporadic_next_exec_a = 1;

volatile int sporadic_pending_b = 0;
volatile int sporadic_count_b   = 0;
volatile int sporadic_next_exec_b = 1;

volatile int sporadic_pending_c = 0;
volatile int sporadic_count_c   = 0;
volatile int sporadic_next_exec_c = 1;

/* Debounce: to avoid one press marking as two jobs entered sometimes */
#define DEBOUNCE_MS  400
volatile uint32_t last_press_tick_a = 0;
volatile uint32_t last_press_tick_b = 0;
volatile uint32_t last_press_tick_c = 0;

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

/* ── EXTI1 initialisation (PA1, falling edge, pull-up, with interrupt) ─────── */
static void EXTI1_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    GPIOA->MODER  &= ~(3U << (1*2));     /* PA1 input */
    GPIOA->PUPDR  &= ~(3U << (1*2));
    GPIOA->PUPDR  |=  (1U << (1*2));     /* pull-up */

    SYSCFG->EXTICR[0] &= ~(0xFU << 4);   /* EXTI1 → Port A */

    EXTI->FTSR |= EXTI_FTSR_TR1;         /* falling edge */
    EXTI->RTSR &= ~EXTI_RTSR_TR1;

    EXTI->IMR |= EXTI_IMR_MR1;

    NVIC_SetPriority(EXTI1_IRQn, 2);
    NVIC_EnableIRQ(EXTI1_IRQn);
}

/* ── EXTI4 initialisation (PA4, falling edge, pull-up, with interrupt) ─────── */
static void EXTI4_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    GPIOA->MODER  &= ~(3U << (4*2));     /* PA4 input */
    GPIOA->PUPDR  &= ~(3U << (4*2));
    GPIOA->PUPDR  |=  (1U << (4*2));     /* pull-up */

    SYSCFG->EXTICR[1] &= ~0xFU;          /* EXTI4 → Port A */

    EXTI->FTSR |= EXTI_FTSR_TR4;         /* falling edge */
    EXTI->RTSR &= ~EXTI_RTSR_TR4;

    EXTI->IMR |= EXTI_IMR_MR4;

    NVIC_SetPriority(EXTI4_IRQn, 2);
    NVIC_EnableIRQ(EXTI4_IRQn);
}

/* PA0 Button — Sporadic Prio 7 */
void EXTI0_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR0)
    {
        EXTI->PR = EXTI_PR_PR0;

        if ((ticks - last_press_tick_a) < DEBOUNCE_MS)
            return;
        last_press_tick_a = ticks;

        sporadic_pending_a++;
        sporadic_count_a++;

        UART_SendString("Sporadic task ");
        { char num[4]; int_to_str(sporadic_count_a, num); UART_SendString(num); }
        UART_SendString(" queued at prio ");
        { char num[4]; int_to_str(SPORADIC_PRIO_A, num); UART_SendString(num); }
        UART_SendString("\r\n");
    }
}

/* PA1 Button (pull-up, falling edge) — Sporadic Prio 5 */
void EXTI1_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR1)
    {
        EXTI->PR = EXTI_PR_PR1;

        if ((ticks - last_press_tick_b) < DEBOUNCE_MS)
            return;
        last_press_tick_b = ticks;

        sporadic_pending_b++;
        sporadic_count_b++;

        UART_SendString("Sporadic task ");
        { char num[4]; int_to_str(sporadic_count_b, num); UART_SendString(num); }
        UART_SendString(" queued at prio ");
        { char num[4]; int_to_str(SPORADIC_PRIO_B, num); UART_SendString(num); }
        UART_SendString("\r\n");
    }
}

/* PA4 Button (pull-up, falling edge) — Sporadic Prio 3 */
void EXTI4_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR4)
    {
        EXTI->PR = EXTI_PR_PR4;

        if ((ticks - last_press_tick_c) < DEBOUNCE_MS)
            return;
        last_press_tick_c = ticks;

        sporadic_pending_c++;
        sporadic_count_c++;

        UART_SendString("Sporadic task ");
        { char num[4]; int_to_str(sporadic_count_c, num); UART_SendString(num); }
        UART_SendString(" queued at prio ");
        { char num[4]; int_to_str(SPORADIC_PRIO_C, num); UART_SendString(num); }
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
static void run_sporadic(int priority, volatile int *pending, int *next_exec)
{
    char line[64];
    char p[4];

    int_to_str(priority, p);

    {
        char id[4];
        int_to_str(*next_exec, id);

        strcpy(line, ">> [Sporadic] task ");
        strcat(line, id);
        strcat(line, " | Prio ");
        strcat(line, p);
        strcat(line, " running\r\n");
    }

    UART_SendString(line);
    delay_ms(350);

    (*next_exec)++;
    (*pending)--;
}

int main(void)
{
    int prio, t;

    SystemInit();
    SysTick_Init();
    UART2_Init();
    EXTI0_Init();
    EXTI1_Init();
    EXTI4_Init();

    while (1)
    {
        for (prio = 0; prio < NUM_PRIORITIES; prio++)
        {
            int sporadic_to_run_a = (SPORADIC_PRIO_A == prio) ? sporadic_pending_a : 0;
            int sporadic_to_run_b = (SPORADIC_PRIO_B == prio) ? sporadic_pending_b : 0;
            int sporadic_to_run_c = (SPORADIC_PRIO_C == prio) ? sporadic_pending_c : 0;

            /* Run the regular tasks for this priority level */
            for (t = 0; t < TASKS_PER_PRIO; t++)
            {
                run_task(prio, prio * TASKS_PER_PRIO + t);
            }

            /* Run only the sporadic jobs that were pending at the start */
            while (sporadic_to_run_a > 0)
            {
                run_sporadic(prio, &sporadic_pending_a, &sporadic_next_exec_a);
                sporadic_to_run_a--;
            }
            while (sporadic_to_run_b > 0)
            {
                run_sporadic(prio, &sporadic_pending_b, &sporadic_next_exec_b);
                sporadic_to_run_b--;
            }
            while (sporadic_to_run_c > 0)
            {
                run_sporadic(prio, &sporadic_pending_c, &sporadic_next_exec_c);
                sporadic_to_run_c--;
            }
        }
    }
}
