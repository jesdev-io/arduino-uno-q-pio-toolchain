# Build your Arduino Uno Q projects without App Lab and ✨automagical✨ business logic!

**▶️ This repo demonstrates how to ◀️**
- ⚡️ Flash **arbitrary programs** from the **QRB2210** MPU to the **STM32U585** MCU without the Arduino App Lab in various ways
- 📥️ Receive **serial messages** from the MCU
- 📤️ **Control the MCU** by sending messages from the MPU

## Requirement table
In general, you need a way to access the Debian OS on your Arduino Nano Q's MPU. You can achieve that over wire with [`adb`](https://docs.arduino.cc/software/app-lab/tutorials/cli/#requirements) or a classic SSH connection if your device is registered on a network. You now have two options to get firmware to the MCU:

| Method         | Requires                            |
| -------------- | ----------------------------------- |
| [Manual OpenOCD](#option-1-generic-local-build-with-manual-transport) | Pre-compiled firmware,               |
| [PIO remote](#option-2-remote-deployment-with-platformio)     | PIO account, local repository clone |
|                |                                     |

> [!IMPORTANT]  
> **Why not build on the MPU??? 😤**  
> You absolutely can. Choose an ARM-compiler that you want and build your software the oldschool way. The MPU already has `arm-zephyr-eabi-gcc` installed. I don't do this because I wanted to use PlatformIO, which installs too much boilerplate when building on the MPU, causing the device to run out of space. Once you compiled your firmware as intended, you can now follow [Option #1](#option-1-generic-local-build-with-manual-transport).

## ⚒️ Building & flashing
The goal of the code and information presented here is to flash an embedded firmware program from the MPU to the MCU without using any of the Arduino App Lab features.

## 🚛 Option #1: Generic local build with manual transport
This option assumes that you already built your firmware with a manual call to `arm-none-eabi-gcc` or `arm-zephyr-eabi-gcc`, the STM32CubeIDE, PlatformIO or any other build chain that targets the STM32U585 and your required libraries. This means that you have a `firmware.elf` file somewhere on your PC.

Transfer this file with either `adb` or `scp` (SSH) as such:
```bash
scp /local/path/firmware.elf user@remote:/home/ardiuino/ # over network
adb push /local/path/firmware.elf /home/ardiuino/ # over USB
```

Additionally, you will need [`QRB2210_swd.cfg`](./shared/QRB2210_swd.cfg) of this repo on your MPU, as it holds the openOCD configuration for the SWD connection between MPU and MPU. You can transfer it similarly:
```bash
scp arduino-uno-q-pio-toolchain/shared/QRB2210_swd.cfg user@remote:/home/ardiuino/ # over network
adb push /local/path/QRB2210_swd.cfg /home/ardiuino/ # over USB
```

*It is possible that this file already exists under a different name on the MPU. I wasn't able to find it.*

You can now flash a program with this command:
```bash
/opt/openocd/bin/openocd -s /opt/openocd/share/openocd/scripts -s /opt/openocd -f /home/arduino/QRB2210_swd.cfg -f /opt/openocd/stm32u5x.cfg -c "program /home/arduino/firmware.elf verify reset exit"
```

If you need a demo program that can send and receive data, check out [main.cpp](./src/main.cpp).

## 🐜 Option #2: Remote deployment with PlatformIO
If building an automated build chain with `adb` or `scp` is not your style, you can use [**PlatformIO remote**](https://docs.platformio.org/en/latest/plus/pio-remote.html) to build locally and flash remotely. This requires you to have a [PlatformIO account](https://docs.platformio.org/en/latest/plus/pio-account.html); it is free and you get some free cloud usage for remote deployment per day. Log into your PlatformIO account on both your local machine and the Arduino Uno Q with
```bash
pio account login
```
and provide your user email and password. You can then start a remote agent on your MPU with
```bash
pio remote agent start
``` 
It will authenticate and keep the terminal to itself, using it for logging of transactions. On your main machine you can now list this open agent with
```bash
pio remote agent list
```
The username of your MPU should pop up.

From inside this repo you can now launch the [demo program](./src/main.cpp) by calling
```bash
pio remote run -t upload
``` 
The project will be built locally and needed resources concerned with flashing will be installed on the MPU. Then, the upload process calls a custom upload commad similar to what you would manually do in [Option #1](#option-1-generic-local-build-with-manual-transport). 

## 🔁 Monitoring (RX) and Control (TX)
According to the [Arduino Uno Q datasheet](https://docs.arduino.cc/resources/datasheets/ABX00162-ABX00173-datasheet.pdf) p. 13, the MPU and MCU are not only connected via the SWD interface for flashing and debugging, but also via SPI and (low power) LPUART. These busses are also accessed by the Arduino Bridge, the piece of software we are currently navigating around. If you use the [demo program](./src/main.cpp), you can see how we can use the correct UART configuration to bring the LPUART under control on the MCU. If we then take a look at the [schematic](https://docs.arduino.cc/resources/schematics/ABX00162-schematics.pdf), we can see that the LPUART's TX and RX lines are connected to the GPIOs 71 and 80 on the MPU, which are then mapped to the `/dev/ttyHS1` device on the MPU (*please open an issue if you really want to find out how I know that, it's complicated...*). 

Since the Arduino Bridge uses this bus as well, we have to make sure that it does not use the serial port for any idle messaging or whatever the Bridge employs. To achieve this, we have to kill a service called `arduino-router` first, as it occasionally takes control of `/dev/ttyHS1` and starts at device boot. We can stop it with
```bash
sudo systemctl stop arduino-router
```
The serial port is now free to use.

If the demo firmware is running on the MCU, you should see a red LED blinking. You can now open `/dev/ttyHS1` with a serial terminal emulator such as `picocom` or any other terminal emulator
```bash
sudo picocom -b 9600 /dev/ttyHS1
``` 
You can now send the characters 1-9 which represent the blinking period in seconds. The LED blink speed will adapt and the program will echo the entered number. 



## ⚠️ Important notes
This repo shows some oddities: 
1. The chosen framework in `platformio.ini` is **Arduino**, yet the code is written in the **STM32Cube HAL**
2. The listed board is the **`disco_b_u585i_iot02a`**, yet the actual chip on the Arduino Uno Q is the **STM32U585AII6TR**

Oddity #1 is because of PlatformIO: It does not yet support the combination of the STM32Cube HAL and the chosen board `disco_b_u585i_iot02a`, it only supports the Arduino FW. This however does not yet abstract the `SerialLP1` correctly, which is the Arduino FW version of LPUART1. For this reason, I use the Arduino FW for the backend build chain but have a firmware that calls STM32Cube HAL functions.

Oddity #2 is also because of PlatformIO. It does not support any arbitrary chip ST puts on the market, of which the one on the Arduino Uno Q (STM32U585AII6TR), is one of them. The closest match to that is the `disco_b_u585i_iot02a` board, which has a chip from the same family (STM32U585). They are close enough to have the same capabilities, although the `disco_b_u585i_iot02a` has a lot of pin mappings to peripherals that of course don't exist on the Uno Q.