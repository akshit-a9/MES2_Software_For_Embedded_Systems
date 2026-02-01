#include "modbus.h"
#include "uart.h"
#include "delay.h"

/* Convert a nibble (0-15) to ASCII hex character ('0'-'9', 'A'-'F') */
static uint8_t Nibble_To_ASCII(uint8_t nibble)
{
    if (nibble < 10)
        return '0' + nibble;
    else
        return 'A' + (nibble - 10);
}

/* Convert a byte to two ASCII hex characters */
static void Byte_To_ASCII(uint8_t byte, uint8_t *ascii_high, uint8_t *ascii_low)
{
    *ascii_high = Nibble_To_ASCII((byte >> 4) & 0x0F);
    *ascii_low  = Nibble_To_ASCII(byte & 0x0F);
}

/* Calculate LRC (Longitudinal Redundancy Check) */
static uint8_t Modbus_LRC(uint8_t *buf, uint16_t len)
{
    uint8_t lrc = 0;
    uint16_t i;
    
    for (i = 0; i < len; i++)
        lrc += buf[i];
    
    /* Two's complement */
    return (uint8_t)(-lrc);
}

/* Send a byte as two ASCII hex characters */
static void Send_Byte_ASCII(uint8_t byte)
{
    uint8_t ascii_high, ascii_low;
    Byte_To_ASCII(byte, &ascii_high, &ascii_low);
    UART_SendByte(ascii_high);
    UART_SendByte(ascii_low);
}

/* Helper function: Write a single register using Function 0x06 */
static void Write_Single_Register_ASCII(uint8_t slave, uint16_t reg_addr, uint16_t reg_value)
{
    uint8_t frame[6];
    uint8_t lrc;
    uint16_t i;

    /* Build the binary frame (for LRC calculation) */
    frame[0] = slave;                           /* Slave address */
    frame[1] = 0x06;                            /* Function code: Write Single Register */
    frame[2] = (uint8_t)((reg_addr >> 8) & 0xFF);   /* Register address high byte */
    frame[3] = (uint8_t)(reg_addr & 0xFF);          /* Register address low byte */
    frame[4] = (uint8_t)((reg_value >> 8) & 0xFF);  /* Register value high byte */
    frame[5] = (uint8_t)(reg_value & 0xFF);         /* Register value low byte */

    /* Calculate LRC */
    lrc = Modbus_LRC(frame, 6);

    /* Send ASCII frame */
    UART_SendByte(':');                         /* Start character */
    
    for (i = 0; i < 6; i++)
        Send_Byte_ASCII(frame[i]);              /* Send frame bytes as ASCII hex */
    
    Send_Byte_ASCII(lrc);                       /* Send LRC as ASCII hex */
    
    UART_SendByte(0x0D);                        /* Carriage Return */
    UART_SendByte(0x0A);                        /* Line Feed */
    
    /* Small delay to allow motor controller to process the command */
    delay_ms(10);
}

void RMCS_SetPosition(uint8_t slave, int32_t pos, uint16_t speed)
{
    uint16_t pos_high, pos_low;
    
    /* Split 32-bit position into two 16-bit registers */
    pos_high = (uint16_t)((pos >> 16) & 0xFFFF);
    pos_low  = (uint16_t)(pos & 0xFFFF);

    /* Write registers according to RMCS-2303 datasheet */
    
    /* Register 2 (40003): Mode - Enable Position Control Mode */
    /* 0x0201 = Mode 2 (position control), Control byte 01 (enable) */
    Write_Single_Register_ASCII(slave, 2, 0x0201);
    
    /* Register 14 (40015): Speed Command in RPM */
    Write_Single_Register_ASCII(slave, 14, speed);
    
    /* Register 12 (40013): Acceleration (using default or custom value) */
    Write_Single_Register_ASCII(slave, 12, 20000);  /* Default acceleration */
    
    /* Register 16 (40017): LSB of Position Command (lower 16 bits) */
    Write_Single_Register_ASCII(slave, 16, pos_low);
    
    /* Register 18 (40019): MSB of Position Command (upper 16 bits) */
    /* IMPORTANT: Motion is triggered when register 18 is updated! */
    Write_Single_Register_ASCII(slave, 18, pos_high);
}
