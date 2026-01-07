#include "stm32f4xx.h"
#include "uart.h"
#include "gpio.h"
#include "modbus.h"
#include "delay.h"

int main(void)
{
    SystemInit();
    SysTick_Init();

    UART2_Init();
    RMCS_GPIO_Init();

    RMCS_BRAKE_OFF();
    RMCS_ENABLE();

    delay_ms(200);

    while (1)
    {
        RMCS_SetPosition(1, 50000, 400);
        delay_ms(3000);

        RMCS_SetPosition(1, -50000, 400);
        delay_ms(3000);
    }
}
