#include "stm32f4xx.h"
#include "gpio.h"

void RMCS_GPIO_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    GPIOB->MODER |= (1<<(0*2)) | (1<<(1*2)) | (1<<(2*2));
}
