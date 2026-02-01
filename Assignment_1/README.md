# RMCS-2303 Motor Controller - STM32F407 Implementation

This project controls the RMCS-2303 motor driver using **Modbus ASCII** protocol over UART from an STM32F407 microcontroller.

## System Overview

- **Microcontroller**: STM32F407VGT6
- **Motor Controller**: RMCS-2303 (Slave ID: 7)
- **Communication Protocol**: Modbus ASCII
- **UART**: USART2 (PA2/PA3) @ 9600 baud
- **Clock**: HSI 16MHz (no PLL)
- **Control Mode**: Position Control Mode (Mode 2)

## What This Does

The motor moves between two positions in a continuous loop:
- Position: **+50,000** encoder counts
- Wait 3 seconds
- Position: **-50,000** encoder counts  
- Wait 3 seconds
- Repeat

Speed: **400 RPM** | Acceleration: **20,000** (default)

---

## Major Changes Made

### 1. Modbus RTU → Modbus ASCII Conversion

**Why:** The original implementation used Modbus RTU, but the RMCS-2303 requires Modbus ASCII for reliable communication.

#### Key Differences

| Feature | RTU (Old) | ASCII (New) |
|---------|-----------|-------------|
| **Encoding** | Binary data | ASCII hex characters |
| **Error Check** | CRC-16 | LRC (Longitudinal Redundancy Check) |
| **Frame Start** | Silent interval | `:` (colon, 0x3A) |
| **Frame End** | Silent interval | `CR LF` (0x0D 0x0A) |
| **Data Size** | 1 byte = 1 byte | 1 byte = 2 ASCII characters |
| **Human Readable** | No | Yes |

#### Implementation Changes

**File**: `motorcontrol/modbus.c`

- Added `Nibble_To_ASCII()` - converts 0-15 to '0'-'9', 'A'-'F'
- Added `Byte_To_ASCII()` - converts byte to two ASCII hex chars
- Replaced `Modbus_CRC()` with `Modbus_LRC()` - two's complement checksum
- Modified frame transmission to include `:` start and `CR LF` end

**Example Frame**:
```
:0706000E019084<CR><LF>
```
- `:` - Start
- `07` - Slave ID 7
- `06` - Function Code (Write Single Register)
- `000E` - Register address 14 (speed)
- `0190` - Value 400 (0x190 = 400 decimal)
- `84` - LRC checksum
- `<CR><LF>` - End

---

### 2. UART Baud Rate Fix

**Why:** The baud rate register value (BRR) was incorrect for the 16MHz HSI clock configuration.

**File**: `motorcontrol/uart.c`

```diff
- USART2->BRR = 0x1117;  /* Wrong value */
+ USART2->BRR = 0x683;   /* Correct for 9600 baud @ 16MHz */
```

**Calculation**:
- Formula: `USARTDIV = f_PCLK / (16 × baud_rate)`
- `USARTDIV = 16,000,000 / (16 × 9600) = 104.1667`
- Mantissa = 104 (0x68)
- Fraction = 0.1667 × 16 ≈ 3 (0x3)
- **BRR = 0x683**

---

### 3. Function Code: 0x10 → 0x06

**Why:** The RMCS-2303 doesn't support Function 0x10 (Write Multiple Registers). Changed to Function 0x06 (Write Single Register).

**Impact**: Instead of one large frame, we now send **5 separate frames** for each position command.

---

### 4. Corrected Register Addresses (CRITICAL)

**Why:** The initial implementation used incorrect register addresses (1-5) that don't exist in the RMCS-2303 register map.

#### Correct Register Map (from RMCS-2303 Datasheet)

| Modbus Addr | Register | Name | Value | Purpose |
|-------------|----------|------|-------|---------|
| 40003 | **2** | Mode | `0x0201` | Enable Position Control Mode |
| 40013 | **12** | Acceleration | `20000` | Motor acceleration |
| 40015 | **14** | Speed | varies | Speed in RPM |
| 40017 | **16** | Position LSB | varies | Lower 16 bits of position |
| 40019 | **18** | Position MSB | varies | Upper 16 bits (**triggers motion**) |

> **Important**: Writing to register 18 (Position MSB) triggers the motor to execute the position command!

#### Register Address Notation

Modbus datasheets use "40001" format, but the protocol uses offset addresses:

```
Protocol Address = Datasheet Address - 40001
```

Examples:
- 40003 → Register 2
- 40015 → Register 14
- 40019 → Register 18

---

### 5. Added Inter-Command Delays

**Why:** The motor controller needs time to process each Modbus command.

**Implementation**: Added `delay_ms(10)` after each `Write_Single_Register` call in `modbus.c`.

---

## File Structure

```
motorcontrol/
├── main.c                    # Main application loop
├── modbus.c                  # Modbus ASCII implementation ✨ NEW
├── modbus.h                  # Modbus function declarations
├── modbus_rtu_backup.c       # Original RTU version (backup)
├── uart.c                    # UART driver ✨ MODIFIED
├── uart.h                    # UART declarations
├── gpio.c                    # Motor enable/brake/direction pins
├── gpio.h                    # GPIO macros
├── delay.c                   # SysTick-based delay functions
└── delay.h                   # Delay declarations
```

---

## How It Works

### Initialization Sequence

1. **SystemInit()** - Configure clocks (HSI 16MHz)
2. **SysTick_Init()** - Setup 1ms timer for delays
3. **UART2_Init()** - Configure USART2 @ 9600 baud
4. **RMCS_GPIO_Init()** - Setup motor control pins (PB0-PB2)
5. **RMCS_BRAKE_OFF()** - Release motor brake (PB1 = LOW)
6. **RMCS_ENABLE()** - Enable motor driver (PB0 = HIGH)

### Position Command Sequence

When `RMCS_SetPosition(7, 50000, 400)` is called:

```c
// 1. Enable Position Control Mode
Write_Single_Register_ASCII(7, 2, 0x0201);

// 2. Set Speed (400 RPM)
Write_Single_Register_ASCII(7, 14, 400);

// 3. Set Acceleration
Write_Single_Register_ASCII(7, 12, 20000);

// 4. Set Position Lower 16 bits
Write_Single_Register_ASCII(7, 16, 0xC350);  // 50000 & 0xFFFF

// 5. Set Position Upper 16 bits (triggers motion!)
Write_Single_Register_ASCII(7, 18, 0x0000);  // 50000 >> 16
```

Each register write sends a complete Modbus ASCII frame:
```
:0706000200201EC<CR><LF>
:07060000E019084<CR><LF>
...
```

---

## Hardware Connections

### UART (Modbus Communication)

| STM32F407 | RMCS-2303 |
|-----------|-----------|
| PA2 (TX)  | RX        |
| PA3 (RX)  | TX        |
| GND       | GND       |

### Motor Control Signals

| STM32F407 | Function | Description |
|-----------|----------|-------------|
| PB0 | ENABLE | HIGH = Motor enabled |
| PB1 | BRAKE | LOW = Brake off |
| PB2 | DIR | Direction control |

---

## Building and Flashing

### Prerequisites
- Keil µVision 5
- ST-Link debugger
- STM32F4xx device support pack

### Steps

1. Open `motorcontrol/mcontrol.uvprojx` in Keil
2. Click **Rebuild** (or press F7)
3. Configure ST-Link:
   - Options → Debug → Use: **ST-Link Debugger**
   - Settings → Port: **SW** (not JTAG)
   - Settings → Connect: **under Reset**
4. Click **Download** (F8) to flash
5. Click **Start Debug** (Ctrl+F5) and **Run** (F5)

---

## Modbus ASCII Frame Format

### General Structure
```
: [SLAVE] [FUNC] [DATA...] [LRC] CR LF
```

### Example: Write Speed Register
```
:07 06 000E 0190 84 <CR><LF>
```

| Field | Value | Hex | Description |
|-------|-------|-----|-------------|
| Start | `:` | 0x3A | Frame start |
| Slave | `07` | - | Slave ID 7 |
| Function | `06` | - | Write Single Register |
| Reg Addr | `000E` | - | Register 14 (speed) |
| Value | `0190` | - | 400 decimal |
| LRC | `84` | - | Checksum |
| End | `<CR><LF>` | 0x0D 0x0A | Frame end |

### LRC Calculation

```c
uint8_t Modbus_LRC(uint8_t *buf, uint16_t len)
{
    uint8_t lrc = 0;
    for (uint16_t i = 0; i < len; i++)
        lrc += buf[i];
    return (uint8_t)(-lrc);  // Two's complement
}
```

---

## Troubleshooting

### Motor Not Moving

1. **Check UART connections** - TX/RX swapped?
2. **Verify slave ID** - Is motor set to ID 7?
3. **Monitor UART output** - Use USB-to-TTL adapter on PA2
4. **Check motor power** - Separate power supply connected?
5. **Verify enable signals**:
   - PB0 (ENABLE) should be 3.3V
   - PB1 (BRAKE) should be 0V

### Debug with Serial Monitor

Connect USB-to-TTL adapter:
- Adapter RX → STM32 PA2
- Adapter GND → STM32 GND
- Terminal: 9600 baud, 8N1

You should see:
```
:0706000200201EC
:07060000E019084
:07060000C4E20F5
:070600100C5028
:07060012000085
```

---

## Mode Values Reference

| Mode | Hex Value | Decimal | Description |
|------|-----------|---------|-------------|
| Position Enable | `0x0201` | 513 | Enable position control |
| Position Disable | `0x0200` | 512 | Disable position mode |
| Digital CW | `0x0101` | 257 | Clockwise rotation |
| Digital CCW | `0x0109` | 265 | Counter-clockwise |
| E-Stop | `0x0700` | 1792 | Emergency stop |
| Stop | `0x0701` | 1793 | Stop and hold |
| Set Home | `0x0800` | 2048 | Zero encoder |

---

## References

- [RMCS-2303 Datasheet](./RMCS-2303%20updated%20datasheet.pdf)
- [Modbus Protocol Specification](./Modbus%20Protocol.pdf)
- STM32F407 Reference Manual (RM0090)

---

## License

Educational project for embedded systems course.

## Author

Created for Assignment 1 - Motor Position Control
