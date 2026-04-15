#include "iic_soft.h"





/**
 * @brief 模拟I2C延时
 *
 */
static void i2c_Delay(void)
{
    for (uint16_t i = 0; i < 250; i++)
        ;
}

/**
 * @brief CPU发起I2C总线启动信号
 *     _____
 * SDA      \_____________
 *     __________
 * SCL           \________
 */
 void i2c_Start(void)
{
    /* 当SCL高电平时，SDA出现一个下跳沿表示I2C总线启动信号 */
    I2C_SDA_High();
    I2C_SCL_High();
    i2c_Delay();
    I2C_SDA_Low();
    i2c_Delay();
    I2C_SCL_Low();
    i2c_Delay();
}

/**
 * @brief CPU发起I2C总线停止信号
 *                _______
 * SDA __________/
 *           ____________
 * SCL _____/
 */
 void i2c_Stop(void)
{
    /* 当SCL高电平时，SDA出现一个上跳沿表示I2C总线停止信号 */
    I2C_SDA_Low();
    I2C_SCL_High();
    i2c_Delay();
    I2C_SDA_High();
}

/*
*********************************************************************************************************
*	函 数 名: i2c_SendByte
*	功能说明: CPU向I2C总线设备发送8bit数据
*	形    参：_ucByte ： 等待发送的字节
*	返 回 值: 无
*********************************************************************************************************
*/
 void i2c_SendByte(uint8_t _ucByte)
{
    uint8_t i;

    /* 先发送字节的高位bit7 */
    for (i = 0; i < 8; i++)
    {
        if (_ucByte & 0x80)
        {
            I2C_SDA_High();
        }
        else
        {
            I2C_SDA_Low();
        }
        i2c_Delay();
        I2C_SCL_High();
        i2c_Delay();
        I2C_SCL_Low();
        if (i == 7)
        {
            I2C_SDA_High(); // 释放总线
        }
        _ucByte <<= 1; /* 左移一个bit */
        i2c_Delay();
    }
}

/*
*********************************************************************************************************
*	函 数 名: i2c_ReadByte
*	功能说明: CPU从I2C总线设备读取8bit数据
*	形    参：无
*	返 回 值: 读到的数据
*********************************************************************************************************
*/
 uint8_t i2c_ReadByte(void)
{
    uint8_t i;
    uint8_t value;

    /* 读到第1个bit为数据的bit7 */
    value = 0;
    for (i = 0; i < 8; i++)
    {
        value <<= 1;
        I2C_SCL_High();
        i2c_Delay();
        if (I2C_SDA_READ())
        {
            value++;
        }
        I2C_SCL_Low();
        i2c_Delay();
    }
    return value;
}

/**
 * @brief CPU产生一个ACK信号
 *     ____(从机应答)¯¯¯¯¯
 * SDA
 *           ____
 * SCL _____/    \________
 */
 uint8_t i2c_WaitAck(void)
{
    uint8_t re;

    I2C_SDA_High(); /* CPU释放SDA总线 */
    i2c_Delay();
    I2C_SCL_High(); /* CPU驱动SCL = 1, 此时器件会返回ACK应答 */
    i2c_Delay();
    if (I2C_SDA_READ()) /* CPU读取SDA口线状态 */
    {
        re = 1;
    }
    else
    {
        re = 0;
    }
    I2C_SCL_Low();
    i2c_Delay();
    return re;
}

/**
 * @brief CPU产生一个ACK信号
 *                  _____
 * SDA ____________/
 *          ____
 * SCL ____/    \________
 */
 void i2c_Ack(void)
{
    I2C_SDA_Low(); /* CPU驱动SDA = 0 */
    i2c_Delay();
    I2C_SCL_High(); /* CPU产生1个时钟 */
    i2c_Delay();
    I2C_SCL_Low();
    i2c_Delay();
    I2C_SDA_High(); /* CPU释放SDA总线 */
}

/**
 * @brief I2C 不响应(CPU产生1个NACK信号)
 *     __________________
 * SDA
 *            ____
 * SCL ______/    \______
 */
static void i2c_NAck(void)
{
    I2C_SDA_High(); /* CPU驱动SDA = 1 */
    i2c_Delay();
    I2C_SCL_High(); /* CPU产生1个时钟 */
    i2c_Delay();
    I2C_SCL_Low();
    i2c_Delay();
}

/**
 * @brief SCL和SDA IO初始化
 * 开漏输出 GPIO_MODE_OUTPUT_OD
 * 不上下拉 GPIO_NOPULL
 * IO最高速度 GPIO_SPEED_FREQ_HIGH
 */
static void i2c_gpio_init(void)
{

    /* 给一个停止信号, 复位I2C总线上的所有设备到待机模式 */
    // i2c_Stop();
}

I2C_Device myi2c = {
    .GPIO_Init = i2c_gpio_init,
    .Start = i2c_Start,
    .Stop = i2c_Stop,
    .SendByte = i2c_SendByte,
    .ReadByte = i2c_ReadByte,
    .WaitAck = i2c_WaitAck,
    .Ack = i2c_Ack,
    .NAck = i2c_NAck,
};




/*****************************自己封装的函数实现******************************/

void BQ76920_ADC_Init(void)
{
    BQ76920_Write_Reg(0x04, 0x18);// 开启ADC和NTC热敏电阻采样
    
}







/**
 * @brief BQ76920 读寄存器函数 (适配 2003, 处理 CRC)
 */
uint8_t BQ76920_Read_Reg(uint8_t reg_addr, uint8_t *pReadData)
{
    uint8_t status = 0; 
    
    // 【修改】获取当前中断状态并关闭全局中断
    uint32_t primask_bit = __get_PRIMASK();
    __disable_irq(); 
    
    // 虽然关了中断，但为了 FreeRTOS 安全，依然保留 Suspend (防止其他 CPU 核或特殊情况)
    vTaskSuspendAll(); 

    i2c_Start();
    i2c_SendByte(0x10); 
    if(i2c_WaitAck() != 0) { status = 1; goto read_exit; }
    
    i2c_SendByte(reg_addr);
    if(i2c_WaitAck() != 0) { status = 1; goto read_exit; }

    i2c_Start(); 
    i2c_SendByte(0x11); 
    if(i2c_WaitAck() != 0) { status = 1; goto read_exit; }

    *pReadData = i2c_ReadByte(); 
    i2c_Ack(); 
    
    uint8_t dummy_crc = i2c_ReadByte(); 
    i2c_NAck(); 
    
    read_exit:
    i2c_Stop();
    xTaskResumeAll(); 
    
    // 【修改】恢复中断状态
    __set_PRIMASK(primask_bit); 
    return status;
}


/*始终开启模式下，CC 以 100% 的功率运行，
每 250 毫秒采集一次新读数。每次读数完成后，
CC_READY 位会被置位，该位会将 ALERT 引脚置高，
以通知微控制器有新的读数可用。要启用始终开启模式，
请将 [CC_EN] 设置为 1
ALERT 是个综合报警引脚：不只是库仑计好了会跳，过压、欠压、短路、温升过高时它都会跳。
状态清除：一旦你读完了电流数据，记得检查并清除 SYS_STAT (0x00) 里的 CC_READY 位，
否则 ALERT 引脚可能会一直保持高电平，导致你没法接收下一次通知。
CHG_ON和DSG_ON：这两个引脚分别用于充电和放电，当它们被设置为 1 时，分别在这个寄存器的第1位和第0位上。
*/


/**
 * @brief 从 BQ76920 开启库伦计数器
 * @param  
 * @param 
 * @return 
 */

 void BQ76920_CC_Init(void)
 {
     // 必须配置 CC_CFG（0-5位） 为 0x19
    BQ76920_Write_Reg(0x0B, 0x19);

    uint8_t temp_val = 0;
    // 使用新的带 CRC 的 Read 函数读取
    if (BQ76920_Read_Reg(0x05, &temp_val) != 0) 
    {
        return;
    }

    temp_val |= 0x40; // 开启 CC_EN (Bit 6)
    
    // 使用新的带 CRC 的 Write 函数写入
    BQ76920_Write_Reg(0x05, temp_val);


   
}


/**
 * 
 * 
 * 
 */

uint8_t BQ769x0_CRC8(uint8_t *ptr, uint8_t len)
{
    uint8_t crc = 0;
    while (len--)
    {
        crc ^= *ptr++;
        for (uint8_t i = 0; i < 8; i++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}



/**
 * @brief BQ76920 写寄存器函数 (适配 2003, 带 CRC)
 */
void BQ76920_Write_Reg(uint8_t reg_addr, uint8_t data)
{
    uint8_t buf[3];
    buf[0] = 0x10;     
    buf[1] = reg_addr; 
    buf[2] = data;     
    uint8_t crc = BQ769x0_CRC8(buf, 3); 

    // 【修改】关中断保护时序
    uint32_t primask_bit = __get_PRIMASK();
    __disable_irq();
    vTaskSuspendAll(); 

    i2c_Start();
    i2c_SendByte(0x10); 
    if(i2c_WaitAck() != 0) goto write_exit;

    i2c_SendByte(reg_addr);
    if(i2c_WaitAck() != 0) goto write_exit;

    i2c_SendByte(data);
    if(i2c_WaitAck() != 0) goto write_exit;

    i2c_SendByte(crc);  
    i2c_WaitAck();

    write_exit:
    i2c_Stop();
    xTaskResumeAll(); 
    
    // 【修改】恢复中断
    __set_PRIMASK(primask_bit);
}





/**
 * @brief BQ76920 连续读寄存器函数 (处理 CRC)
 */
 uint8_t BQ76920_Read_Burst(uint8_t reg_addr, uint8_t pReadData[], uint8_t len)
{
    uint32_t primask_bit = __get_PRIMASK();
    __disable_irq();
    vTaskSuspendAll(); 

    i2c_Start();
    i2c_SendByte(0x10);   // 写地址 (0x08 << 1)
    if(i2c_WaitAck() != 0) goto read_error;
    
    i2c_SendByte(reg_addr);
    if(i2c_WaitAck() != 0) goto read_error;

    i2c_Start(); 
    i2c_SendByte(0x11);   // 读地址 (0x08 << 1 | 1)
    if(i2c_WaitAck() != 0) goto read_error;

    for (uint8_t i = 0; i < len; i++)
    {
        // 1. 读取数据字节
        pReadData[i] = i2c_ReadByte();
        i2c_Ack();                     // 应答数据字节，通知从机发送 CRC
        
        // 2. 读取从机发送的 CRC 字节
        uint8_t recv_crc = i2c_ReadByte();
        
        // 3. 计算期望的 CRC
        uint8_t expected_crc;
        if (i == 0) {
            // 第一个数据字节：CRC 基于 (读地址 + 第一个数据字节)
            uint8_t crc_data[2] = {0x11, pReadData[i]};
            expected_crc = BQ769x0_CRC8(crc_data, 2);
        } else {
            // 后续数据字节：CRC 仅基于当前数据字节
            expected_crc = BQ769x0_CRC8(&pReadData[i], 1);
        }
        
        // 4. 校验 CRC
        if (expected_crc != recv_crc) {
            goto crc_error;   // CRC 校验失败
        }
        
        // 5. 对 CRC 字节发送应答（控制是否继续）
        if (i == len - 1)
            i2c_NAck();       // 最后一个 CRC 后发 NACK
        else
            i2c_Ack();        // 非最后一个 CRC 后发 ACK
    }

    i2c_Stop();
    xTaskResumeAll();
    __set_PRIMASK(primask_bit);
    return 0;   // 成功

crc_error:
read_error:
    i2c_Stop();
    xTaskResumeAll();
    __set_PRIMASK(primask_bit);
    return 1;   // 失败
}





