#pragma once

#include "main.h"
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"



/* 定义I2C总线连接的GPIO端口, 用户只需要修改下面4行代码即可任意改变SCL和SDA的引脚 */
#define I2C_SCL_GPIO_PORT GPIOB
#define I2C_SCL_PIN GPIO_PIN_14

#define I2C_SDA_GPIO_PORT GPIOB
#define I2C_SDA_PIN GPIO_PIN_13

#define I2C_SCL_High() I2C_SCL_GPIO_PORT->BSRR = I2C_SCL_PIN /* SCL = 1 */
#define I2C_SCL_Low() I2C_SCL_GPIO_PORT->BRR = I2C_SCL_PIN   /* SCL = 0 */

#define I2C_SDA_High() I2C_SDA_GPIO_PORT->BSRR = I2C_SDA_PIN /* SDA = 1 */
#define I2C_SDA_Low() I2C_SDA_GPIO_PORT->BRR = I2C_SDA_PIN   /* SDA = 0 */

#define I2C_SDA_READ() ((I2C_SDA_GPIO_PORT->IDR & I2C_SDA_PIN) != 0) /* 读SDA口线状态 */

typedef struct I2C_Device
{    // 函数指针
    void (*GPIO_Init)(void);
    void (*Start)(void);
    void (*Stop)(void);
    void (*SendByte)(uint8_t data);
    uint8_t (*ReadByte)(void);
    uint8_t (*WaitAck)(void);
    void (*Ack)(void);
    void (*NAck)(void);
} I2C_Device;

/*给外面用的函数*/
void i2c_Start(void);
void i2c_Stop(void);
void i2c_SendByte(uint8_t _ucByte);
uint8_t i2c_WaitAck(void);
uint8_t i2c_ReadByte(void);
void BQ76920_ADC_Init(void);                                      // 初始化ADC寄存器
void BQ76920_CC_Init(void);                                      // 初始化库伦计数器寄存器
uint8_t BQ76920_Read_Reg(uint8_t reg_addr, uint8_t *pReadData);  // 通过i2c协议读取寄存器函数
uint8_t BQ769x0_CRC8(uint8_t *ptr, uint8_t len);                 // 计算CRC8校验和
void BQ76920_Write_Reg(uint8_t reg_addr, uint8_t data);          // 通过i2c协议写入寄存器函数
uint8_t BQ76920_Read_Burst(uint8_t reg_addr, uint8_t *pReadData, uint8_t len); // 通过i2c协议连续读取寄存器函数 (适配 CRC 模式)

extern I2C_Device myi2c;
