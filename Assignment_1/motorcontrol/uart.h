#ifndef UART_H
#define UART_H

#include <stdint.h>

void UART2_Init(void);
void UART_SendByte(uint8_t data);
void UART_SendArray(uint8_t *buf, uint16_t len);

#endif
