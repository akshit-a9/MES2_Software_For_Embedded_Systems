#ifndef MODBUS_H
#define MODBUS_H

#include <stdint.h>

void RMCS_SetPosition(uint8_t slave, int32_t pos, uint16_t speed);

#endif
