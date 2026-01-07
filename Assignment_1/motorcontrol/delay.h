#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>     /* REQUIRED*/
extern volatile uint32_t ticks;

void SysTick_Init(void);
void delay_ms(uint32_t ms);

/* IRQ handler prototype */
void SysTick_Handler(void);

#endif
