#include "modbus.h"
#include "uart.h"

static uint16_t Modbus_CRC(uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;
	  uint16_t i;
		uint8_t j;
    for (i = 0; i < len; i++)
    {
        crc ^= buf[i];
			  
        for (j = 0; j < 8; j++)
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
    }
    return crc;
}

void RMCS_SetPosition(uint8_t slave, int32_t pos, uint16_t speed)
{
    uint8_t frame[17];
    uint16_t crc;

    frame[0] = slave;
    frame[1] = 0x10;
    frame[2] = 0x00;
    frame[3] = 0x01;
    frame[4] = 0x00;
    frame[5] = 0x05;
    frame[6] = 0x0A;

    frame[7] = 0x00;
    frame[8] = 0x01;

		frame[9]  = (uint8_t)((pos >> 24) & 0xFF);
		frame[10] = (uint8_t)((pos >> 16) & 0xFF);
		frame[11] = (uint8_t)((pos >> 8)  & 0xFF);
		frame[12] = (uint8_t)(pos & 0xFF);

    frame[13] = (uint8_t)((speed >> 8) & 0xFF);
		frame[14] = (uint8_t)(speed & 0xFF);

    frame[15] = 0x00;
    frame[16] = 0x01;

    crc = Modbus_CRC(frame, 17);

    UART_SendArray(frame, 17);
    UART_SendByte(crc & 0xFF);
    UART_SendByte(crc >> 8);
}
