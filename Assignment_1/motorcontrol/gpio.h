#ifndef GPIO_H
#define GPIO_H

void RMCS_GPIO_Init(void);

#define RMCS_ENABLE()    (GPIOB->ODR |=  (1U<<0))
#define RMCS_DISABLE()   (GPIOB->ODR &= ~(1U<<0))

#define RMCS_BRAKE_ON()  (GPIOB->ODR |=  (1U<<1))
#define RMCS_BRAKE_OFF() (GPIOB->ODR &= ~(1U<<1))

#define RMCS_DIR_CW()    (GPIOB->ODR |=  (1U<<2))
#define RMCS_DIR_CCW()   (GPIOB->ODR &= ~(1U<<2))

#endif
