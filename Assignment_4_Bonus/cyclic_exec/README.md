# Assignment 4 — Cyclic Executive with Interrupt-Driven Sporadic Tasks

## Overview

This project demonstrates a **cyclic executive scheduler** running on the **STM32F407VG Discovery board**. It cycles through 50 regular tasks (10 priority levels × 5 tasks each) from lowest priority (0) to highest (9). When the **blue User button** is pressed, an external interrupt injects a **sporadic task at fixed priority 3** into the schedule.

All output is sent over **UART** to your PC via a **USB-to-UART adapter** so you can watch the scheduler in real time.

---

## 1. Hardware Required

| Item | Notes |
|------|-------|
| STM32F407VG Discovery board | The one with 4 LEDs and the blue User button |
| USB-to-UART adapter (e.g. CP2102, CH340, FT232RL) | 3.3 V logic level — **do NOT use a 5 V-only adapter** |
| Mini-USB cable | To power the Discovery board (connects to the ST-LINK USB connector on top) |
| 3 × Female-to-Female jumper wires | For TX, RX (optional), and GND |
| PC with Windows | With Keil µVision 5 installed |

---

## 2. Wiring — STM32 Discovery → USB-to-UART Adapter

The STM32F407 Discovery board has **two rows of pin headers** on each side. We use **USART2** which is mapped to **Port A** pins.

### Pin Mapping

| STM32 Pin | Header Label | USB-UART Adapter Pin | Wire Colour (suggestion) |
|-----------|-------------|---------------------|--------------------------|
| **PA2** (USART2 TX) | Pin labelled **PA2** on the board | **RX** on the adapter | **Yellow** or **Green** |
| **GND** | Any pin labelled **GND** | **GND** on the adapter | **Black** |
| PA3 (USART2 RX) — *optional* | Pin labelled **PA3** | TX on the adapter | Not needed for this project |

### Step-by-Step Wiring

1. **Locate PA2** on the Discovery board:
   - Look at the **left header** (when the USB connectors face up).
   - PA2 is in the row of pins along the left edge. It is clearly labelled on the board silkscreen.

2. **Locate a GND pin**:
   - There are several GND pins on both headers. Pick any one that is convenient.

3. **Connect wires**:
   - `PA2 (STM32)` → `RX (USB-UART adapter)` — this carries serial data **from** the board **to** your PC.
   - `GND (STM32)` → `GND (USB-UART adapter)` — **critical** — both devices must share a common ground.

4. **Plug the USB-UART adapter** into a USB port on your PC.

5. **Plug the mini-USB cable** into the **top** USB connector on the Discovery board (ST-LINK side) to power it and enable programming.

> **⚠️ Do NOT connect VCC/3V3 between the adapter and the board** — each device is already powered by its own USB connection. Connecting power lines together can damage components.

### Wiring Diagram (Text)

```
STM32F407 Discovery             USB-to-UART Adapter
┌──────────────────┐             ┌─────────────────┐
│                  │             │                 │
│  PA2 (TX) ───────────────────► RX               │
│                  │             │                 │
│  GND ────────────────────────► GND              │
│                  │             │                 │
└──────────────────┘             └─────────────────┘
                                       │
                                       │ USB
                                       ▼
                                     Your PC
```

---

## 3. Software Setup — Install Tools

### 3.1 Keil µVision 5

If not already installed:
1. Go to [keil.com/download](https://www.keil.com/download/product/) and download **MDK-ARM** (the free Community edition supports up to 32 KB code).
2. Run the installer and follow the defaults.
3. After installation, Keil will open the **Pack Installer**. Install these packs:
   - **Keil::STM32F4xx_DFP** (Device Family Pack for STM32F4)
   - **ARM::CMSIS** (if not already installed)

### 3.2 ST-LINK USB Driver

1. Download from [st.com](https://www.st.com/en/development-tools/stsw-link009.html) — search for **STSW-LINK009**.
2. Install it so Windows recognises the Discovery board's on-board ST-LINK programmer.

### 3.3 Serial Terminal

Install one of these (all free):
- **PuTTY** — [putty.org](https://www.putty.org/)
- **Tera Term** — [osdn.net/projects/ttssh2](https://osdn.net/projects/ttssh2/)
- **RealTerm** or the built-in **Windows Terminal** with a serial profile.

---

## 4. Create the Keil Project — Step by Step

### Step 1: Launch Keil and Create a New Project

1. Open **Keil µVision 5**.
2. Go to **Project → New µVision Project…**
3. Navigate to the folder:
   ```
   D:\Source_Codes\01_Ongoing\02_Software_For_Embedded_Systems\Assignment_4\cyclic_exec
   ```
4. Name the project file: `cyclic_exec`
5. Click **Save**.

### Step 2: Select the Device

1. In the device-selection dialog that appears, expand:
   **STMicroelectronics → STM32F4 Series → STM32F407 → STM32F407VGTx**
2. Select **STM32F407VGTx** and click **OK**.

### Step 3: Manage Run-Time Environment

A dialog titled **Manage Run-Time Environment** opens.

1. **Check** (tick) the following items:
   - `CMSIS → CORE`
   - `Device → Startup`
2. Click **OK**.

> This adds the startup assembly file and system initialisation code that the MCU needs to boot.

### Step 4: Add Source Files to the Project

1. In the **Project** pane (left side), you will see **Target 1 → Source Group 1**.
2. **Right-click** on **Source Group 1** → **Add Existing Files to Group 'Source Group 1'…**
3. In the file dialog, change the file-type filter to **All Files (*.*)** so you can see `.c` and `.h` files.
4. **Select and add** all of these files (they are in the `cyclic_exec` folder):
   - `main.c`
   - `uart.c`
   - `uart.h`
   - `delay.c`
   - `delay.h`
5. Click **Add**, then **Close**.

### Step 5: Configure the Compiler (AC6 / ARMCLANG)

1. Go to **Project → Options for Target 'Target 1'…** (or press **Alt+F7**).
2. Go to the **Target** tab:
   - Ensure the **Xtal (MHz)** field reads `8.0`.
   - Make sure **ARM Compiler** is set to **Use default compiler version 6** (AC6 / ARMCLANG).
3. Go to the **C/C++ (AC6)** tab:
   - Set **Language C** to **gnu11** (or `c99`).
   - Set **Optimization** to **-O1** (or leave as default).
   - Under **Misc Controls**, add: `-w` (suppresses warnings — optional).
4. Click **OK**.

### Step 6: Configure the Debugger for ST-LINK

1. Go to **Project → Options for Target 'Target 1'…** → **Debug** tab.
2. On the **right side** (Use:), select **ST-Link Debugger** from the dropdown.
3. Click the **Settings** button next to the dropdown.
4. In the ST-Link settings dialog:
   - **Port**: `SW` (Serial Wire).
   - You should see your Discovery board detected (serial number shown).
   - Under the **Flash Download** tab:
     - Make sure **Programming Algorithm** shows `STM32F4xx 1024kB Flash`.
     - Check **Reset and Run** ← *important so the board starts running after flashing*.
5. Click **OK** twice to close both dialogs.

### Step 7: Build the Project

1. Press **F7** (or go to **Project → Build Target**).
2. The **Build Output** window at the bottom should show:
   ```
   "cyclic_exec.axf" - 0 Error(s), 0 Warning(s).
   ```
3. If you see errors, double-click the error message to jump to the problematic line and fix any issues.

### Step 8: Flash the Firmware

1. Make sure the Discovery board is connected via the mini-USB cable (top ST-LINK connector).
2. Press **F8** (or go to **Flash → Download**).
3. You should see:
   ```
   Flash Load finished at ...
   ```
4. If "Reset and Run" was checked, the board will immediately start executing.

---

## 5. Observe the Output

### Step 1: Find the COM Port

1. Open **Device Manager** on your PC (press `Win+X` → Device Manager).
2. Expand **Ports (COM & LPT)**.
3. Look for your USB-to-UART adapter — it will show as something like:
   - `USB-SERIAL CH340 (COM3)`
   - `Silicon Labs CP210x (COM5)`
   - `USB Serial Port (COM7)`
4. Note the **COM port number** (e.g., COM3).

### Step 2: Open the Serial Terminal

#### Using PuTTY:
1. Open PuTTY.
2. Select **Serial** as the connection type.
3. Enter the COM port (e.g., `COM3`) in the **Serial line** field.
4. Set **Speed** to `9600`.
5. Click **Open**.

#### Using Tera Term:
1. Open Tera Term.
2. Select **Serial** and choose your COM port.
3. Go to **Setup → Serial Port** and set:
   - Baud rate: `9600`
   - Data: `8 bit`
   - Parity: `None`
   - Stop bits: `1`
   - Flow control: `None`
4. Click **OK**.

### Step 3: Watch and Interact

You should see output scrolling like this:

```
Priority 0 : task 0 executes
Priority 0 : task 1 executes
Priority 0 : task 2 executes
Priority 0 : task 3 executes
Priority 0 : task 4 executes
Priority 1 : task 5 executes
Priority 1 : task 6 executes
...
Priority 9 : task 49 executes
Priority 0 : task 0 executes    ← cycle repeats
...
```

**Now press the blue User button:**

```
Priority 3 : task 17 executes
Priority 3 : task 18 executes
Sporadic job added to queue (priority 3)     ← button pressed here
Priority 3 : task 19 executes
[Sporadic job] Priority 3 : sporadic task executes
Priority 4 : task 20 executes
...
```

If you press the button **after** priority 3 has passed in the current cycle:

```
Priority 7 : task 37 executes
Sporadic job added to queue (priority 3)     ← button pressed here
Priority 7 : task 38 executes
...
Priority 9 : task 49 executes
Priority 0 : task 0 executes                ← next cycle starts
...
Priority 3 : task 19 executes
[Sporadic job] Priority 3 : sporadic task executes   ← runs in next cycle
Priority 4 : task 20 executes
```

---

## 6. Project File Structure

```
Assignment_4/
└── cyclic_exec/
    ├── main.c          ← Cyclic executive + EXTI0 ISR (blue button)
    ├── uart.c          ← USART2 driver (PA2/PA3, 9600 baud)
    ├── uart.h          ← UART function declarations
    ├── delay.c         ← SysTick 1ms tick + delay_ms()
    ├── delay.h         ← Delay function declarations
    └── cyclic_exec.uvprojx   ← Keil project file (created by you in Step 4)
```

---

## 7. Troubleshooting

| Problem | Solution |
|---------|----------|
| No output on terminal | Check PA2→RX wiring and GND connection. Verify correct COM port. |
| Garbled text | Baud rate mismatch. Ensure terminal is set to **9600, 8N1**. |
| Build error: "cannot find stm32f4xx.h" | Make sure the CMSIS and STM32F4 DFP packs are installed (Step 3.1). |
| Flash download fails | Install ST-LINK driver (Step 3.2). Check USB cable is in the **top** connector. |
| Button press not detected | The blue button is on the **left side** of the board, labelled **B1 / USER**. Make sure you're not pressing the black RESET button. |
| Sporadic job never appears | Check the wiring and that PA0 is not shorted to anything. The blue button should be connected to PA0 by default on the Discovery board. |

---

## 8. How It Works (Concept)

```
┌─────────────────────────────────────────────────────┐
│                    CYCLIC EXECUTIVE                  │
│                                                     │
│  for priority = 0 to 9:                             │
│      for each task in priority group:               │
│          execute task (print + delay)                │
│                                                     │
│      if sporadic_pending AND priority == 3:          │
│          execute sporadic task                       │
│          clear sporadic_pending                      │
│                                                     │
│  repeat forever                                     │
└─────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────┐
│                   EXTI0 ISR                          │
│             (Blue button press)                      │
│                                                     │
│  Set sporadic_pending = 1                            │
│  Print "Sporadic job added to queue (priority 3)"    │
└─────────────────────────────────────────────────────┘
```

The key insight is that the sporadic job is **not preemptive** — the interrupt only sets a flag. The cyclic executive **polls** this flag at the end of each priority group and only runs the sporadic task when it reaches the matching priority level (3).
