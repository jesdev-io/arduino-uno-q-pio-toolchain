/*
This demo firmware blinks the red LED on the Arduino Uno Q connected
to the STM32U585 MCU. It can also receive the characters 1 to 9 which
then determine the blinking speed, which it also echoes on change.
The point of this firmware is to demonstrate how the MCU can run any
program without the usage and middleware that the Arduino App Lab may
inject. This brings the program back to a bare-metal state if desired:
No Zephyr-OS, not even necessarily a library or framework. 
*/

#include <Arduino.h>
#include "stm32u5xx_hal.h"

UART_HandleTypeDef hlpuart1;

void error_handler(uint32_t speed);

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE4) != HAL_OK){
        error_handler(1);
    }
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK){
        error_handler(1);
    }
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                                |RCC_CLOCKTYPE_PCLK3;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK){
        error_handler(1);
    }
}

static void MX_LPUART1_UART_Init(void){
    hlpuart1.Instance = LPUART1;
    hlpuart1.Init.BaudRate = 9600;
    hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
    hlpuart1.Init.StopBits = UART_STOPBITS_1;
    hlpuart1.Init.Parity = UART_PARITY_NONE;
    hlpuart1.Init.Mode = UART_MODE_TX_RX;
    hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    hlpuart1.FifoMode = UART_FIFOMODE_DISABLE;
    if (HAL_UART_Init(&hlpuart1) != HAL_OK){
        error_handler(1);
    }
    if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK){
        error_handler(1);
    }
    if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK){
        error_handler(1);
    }
    if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK){
        error_handler(1);
    }
}

static void MX_GPIO_Init(void){
    __HAL_RCC_GPIOG_CLK_ENABLE();
}

void HAL_MspInit(void){
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWREx_EnableVddIO2();
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart){
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    if(huart->Instance==LPUART1){
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPUART1;
        PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK3;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK){
            error_handler(1);
        }
        __HAL_RCC_LPUART1_CLK_ENABLE();
        __HAL_RCC_GPIOG_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF8_LPUART1;
        HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* huart){
    if(huart->Instance==LPUART1){
        __HAL_RCC_LPUART1_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOG, GPIO_PIN_7|GPIO_PIN_8);
    }
}

void setup() {
    pinMode(PH10, OUTPUT);
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_LPUART1_UART_Init();
    char msg[] = "Change blink speed from 1 to 9!\n\r";
    HAL_UART_Transmit(&hlpuart1, (uint8_t*)msg, sizeof(msg), HAL_MAX_DELAY);
}

void loop() {
    uint8_t rxByte;
    static uint32_t blinkSpeed = 1000;
    if (HAL_UART_Receive(&hlpuart1, &rxByte, 1, 100) == HAL_OK) {
        char buf[] = {rxByte, '\n', '\r'};
        HAL_UART_Transmit(&hlpuart1, (uint8_t*)buf, sizeof(buf), HAL_MAX_DELAY);
        if (rxByte >= '0' && rxByte <= '9') {
        blinkSpeed = (rxByte - '0') * 1000;
        }
    }
    digitalWrite(PH10, HIGH);
    delay(blinkSpeed / 2);
    digitalWrite(PH10, LOW);
    delay(blinkSpeed / 2);
}

void error_handler(uint32_t speed) {
    while (1) {
        digitalWrite(PH10, HIGH);
        delay(speed*1000);
        digitalWrite(PH10, LOW);
        delay(speed*1000);
    }
}