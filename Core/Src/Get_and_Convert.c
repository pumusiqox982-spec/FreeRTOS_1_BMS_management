#include "Get_and_Convert.h"


/*需要的头文件*/
#include "Safety_Protection.h"






/*全局变量*/
// 全局变量
// 全局数据实例，供其他文件通过 extern 使用
g_bq_status BQ76920_Data = {0}; //分配内存，要是只声明不给赋值，会报错，在这里相当于是给内存赋值为0



// 三元锂电池 SOC开路电压法计算数据表
// 支持磷酸铁锂、钛酸锂也得做一张这个表
uint16_t SocOcvTab[101]=
{
	3282, // 0%~1%	
	3309, 3334, 3357, 3378, 3398, 3417, 3434, 3449, 3464, 3477,	// 0%~10%
	3489, 3500, 3510, 3520, 3528, 3536, 3543, 3549, 3555, 3561,	// 11%~20%
	3566, 3571, 3575, 3579, 3583, 3586, 3590, 3593, 3596, 3599,	// 21%~30%
	3602, 3605, 3608, 3611, 3615, 3618, 3621, 3624, 3628, 3632,	// 31%~40%
	3636, 3640, 3644, 3648, 3653, 3658, 3663, 3668, 3674, 3679,	// 41%~50%
	3685, 3691, 3698, 3704, 3711, 3718, 3725, 3733, 3741, 3748,	// 51%~60%
	3756, 3765, 3773, 3782, 3791, 3800, 3809, 3818, 3827, 3837,	// 61%~70%
	3847, 3857, 3867, 3877, 3887, 3897, 3908, 3919, 3929, 3940,	// 71%~80%
	3951, 3962, 3973, 3985, 3996, 4008, 4019, 4031, 4043, 4055,	// 81%~90%
	4067, 4080, 4092, 4105, 4118, 4131, 4145, 4158, 4172, 4185,	// 91~100%
};




/*******************************************此文件作用，流程解析**************************************************** */
/*
    1. 从BQ76920读取vc1-vc5的电压值，因为ADC返回14位，所以把他转化为正常数值，公式为：(ADC * GAIN / 1000) + OFFSET
    2. 从BQ76920读取偏移量和增益，公式为：GAIN = 365（μV/LSB） + (ADCGAIN<4:0>in decimal)(1 μV/LSB)
    3. 从BQ76920读取偏移量，公式为：OFFSET = (ADCOFFSET<7:0>in decimal)(1 μV/LSB)
    4. 已经测出vc1-vc5的电压值，接下来写一个函数找出最大和最小的电芯，并且记录下他们的索引，同时计算压差
    5. CC库仑计是用来检查检流电阻两边流过的电流，所以通过读取cc库仑计的寄存器值，推算电流并触发保护
    6. CC库仑计是16位ADC并且寄存器里面是高八位和第八位储存数据，需要合并
    7. 从BQ76920读取热敏电阻的阻值
    8. 从热敏电阻的阻值表中读取温度值
    9. 计算SOC值



*/                                 
                                  








/**
 *  从 BQ76920 读取寄存器值
 *  获取vc1-vc5的电压值
 * 通过读取寄存器VC1_HI和VC1_LO，一到五都读出来
 * 寄存器地址：0x0C-0x0D  0x0E-0x0F  0x10-0x11  0x12-0x13  0x14-0x15
 * iic只能读8位，要分两次发送读取，vc读出来的值是14位
 * 因为VC1_HI和VC1_LO是作为高6位和低8位，所以需要将它们合并起来
 * 合并时，需要屏蔽掉高6位的bit6和bit7，因为它们是0，所以需要0x3F来屏蔽掉高6位的bit6和bit7
 *最后读取出来的是14位的电压值
 * 电压值是微伏级的，要转换成mv级，计算公式是：(ADC * GAIN / 1000) + OFFSET
 * 先放大1000倍到mv，读出来是3700.57mv，强转为整数，得到3700mv
 */

 void BQ76920_Get_Voltage(uint16_t *pVoltage)  //给一个地址，返回5个电压值，每个数组存放一个精确电压值pVoltage[0-4]
 {
    uint8_t hi_byte, lo_byte;//高6位和低8位的电压值
    uint16_t adc_raw;//14位电压值
    for(uint8_t i = 0; i < 5; i++)
    {
        //给hi，和lo分别一个地址
        uint8_t HI = 0;
        uint8_t LO = 0;
        //读取高6位和低8位的电压值
        hi_byte = BQ76920_Read_Reg(0x0C + (i * 2), &HI);
        lo_byte = BQ76920_Read_Reg(0x0D + (i * 2), &LO);
        //合并高6位和低8位
        if(hi_byte == 0 && lo_byte == 0)
        {
            //  拼接 14 位原始值,因为高六位只读取到六位，bit6和bit7为0，所以需要0x3F来屏蔽掉高6位的bit6和bit7
            // 然后高六位左移八位，剩下第八位跟低8位合并（或|）起来，得到14位的电压值
            adc_raw = ((uint16_t)(HI & 0x3F) << 8) | LO;
            // 公式：(ADC * GAIN / 1000) + OFFSET
            float voltage = ((float)adc_raw * BQ76920_Data.GAIN/1000.0f) + (float)BQ76920_Data.OFFSET;
            //  把算出来的 mV 存入数组（强转为整数，比如 3.7V 存为 3700）
            pVoltage[i] = (uint16_t)voltage;
        }
        else
        {
            /*读取失败*/
            pVoltage[i] = 0;
        }
    }
 }




 /**
  * 获取bq79620中的Offset 和 Gain
  * 因为手册说 LSB 大约是 382uv，但每颗芯片在出厂时都有微小偏差
  * 。如果不使用芯片内部存储的专用Offset 和 Gain，你的电压测量可能会有几十甚至上百毫伏的误差。
  * 公式是：voltage(uv) = (adc_raw * GAIN) + OFFSET*1000
  * GAIN 有两个寄存器，ADCGAIN1和ADCGAIN2，但是只需要读ADCGAIN1的Bit 3 和 Bit 2位（(0x50)
  * ADCGAIN2的Bit 7 到 Bit 5位(0x59),其他位置都被占用,所以加起来寄存器是五位
  * 所以增益范围是：365 ~ 396 μV/LSB。
  * GAIN的公式是：GAIN = 365（μV/LSB） + (ADCGAIN<4:0>in decimal)(1 μV/LSB)
  * OFFSET 是寄存器0x51开始，正常寄存器，八位,手册规定 OFFSET 是有符号的补码，单位为mv
  */
 void BQ76920_Get_Offset_Gain(void)  
 {
    uint8_t G1 = 0;   //存放ADCGAIN1寄存器
    uint8_t G2 = 0;   //存放ADCGAIN2寄存器
    uint8_t OT = 0;   //存放OFFSET寄存器

    BQ76920_Read_Reg(0x50, &G1);  //读取ADCGAIN1寄存器
    BQ76920_Read_Reg(0x51, &OT);  //读取OFFSET寄存器
    BQ76920_Read_Reg(0x59, &G2);  //读取ADCGAIN2寄存器


    /*把GAIN合并起来,G1高位，G2低位*/
    uint8_t GAIN_decimal = (G1 & 0x0C) << 1 | (G2 & 0xE0) >> 5;

    /*计算GAIN*/
    BQ76920_Data.GAIN = 365.0f + (float)GAIN_decimal;
    BQ76920_Data.OFFSET = (int8_t)OT;
 }




 /**找出最大和最小的电芯，并且记录下他们的索引
  * 
  */
 void Bms_Find_Voltage_Extremes(void)
{
    // 1. 初始化：先假设第 0 节电芯就是当前的最大和最小值
    // 这一步解决了 MinVolt 为 0 导致判断不生效的问题
    BQ76920_Data.MaxVolt  = (float)BQ76920_Data.Cell_V[0];
    BQ76920_Data.MaxIndex = 0;
    BQ76920_Data.MinVolt  = (float)BQ76920_Data.Cell_V[0];
    BQ76920_Data.MinIndex = 0;

    // 2. 从第 1 节电芯开始遍历对比（索引从 1 到 4）
    for(uint8_t i = 1; i < 5; i++)
    {
        // 找最大值
        if(BQ76920_Data.Cell_V[i] > BQ76920_Data.MaxVolt)
        {
            BQ76920_Data.MaxVolt = BQ76920_Data.Cell_V[i];
            BQ76920_Data.MaxIndex = i;
        }
        
        // 找最小值（排除掉 0V 的情况，防止因采集断路误报）
        if(BQ76920_Data.Cell_V[i] < BQ76920_Data.MinVolt)
        {
            BQ76920_Data.MinVolt = BQ76920_Data.Cell_V[i];
            BQ76920_Data.MinIndex = i;
        }
    }

    // 3. 计算压差（这是 BMS 保护逻辑的核心数据）
    BQ76920_Data.DiffVolt = BQ76920_Data.MaxVolt - BQ76920_Data.MinVolt;
}






/**
 * @brief 获取CC库仑计的电压值
 * @param 获取完之后合并高八位和低八位，得到16位电压值
 * 跟CC库仑计相关的寄存器和位：
 * 1. 数据存储寄存器CC_HI (地址 0x32)：存储库仑计结果的高 8 位 <15:8>。
 * 2. 数据存储寄存器 CC_LO (地址 0x33)：存储库仑计结果的低 8 位 <7:0>。这两个寄存器需要连续读取
 * 3. 状态监控位（判断数据是否就绪）SYS_STAT (地址 0x00) 中的 CC_READY (Bit 7)：
 * 作用：告知单片机“新的电流数据已经算好了，快来读”。
 * 状态：1 表示有新数据；0 表示没准备好或已被清除。
 * 操作：读完数据后，你必须向该位写 1 来手动清除它，否则它会一直保持为 1。
 * 注意：SYS_STAT (0x00) 寄存器的逻辑不是普通的“存储”，而是 W1C (Write 1 to Clear)，即写 1 清零该位。不想清零的直接写0
 * 4. 控制与使能位（开关 CC）：SYS_CTRL2 (地址 0x05) 中的 CC_EN (Bit 6)： 
   作用：库仑计的总开关。   
   操作：设置为 1 开启“持续模式”（每 250ms 更新一次数据）；设置为 0 关闭。
 * 5. 控制与使能位（开关 CC）：SYS_CTRL2 (地址 0x05) 中的 CC_ONESHOT (Bit 5)：
   作用：单次采样模式。在 CC_EN=0 时，把这一位置 1，芯片会只测一次电流（耗时 250ms），测完后自动停止。
   6. 强制配置寄存器（必须初始化的位）：CC_CFG (地址 0x0B)：
      要求：手册明确标注 “Must be programmed to 0x19”（必须编程为 0x19）。
      意义：这是芯片内部对库仑计电路的特定配置，如果不写这个值，电流测量可能不准。

    
     



 * */

 void BQ76920_Get_CC(void)
{
    uint8_t status = 0;
    uint8_t raw_data[2] ; // 准备存放 0x32(HI) 和 0x33(LO)
    int16_t new_cc_raw = 0;

    // 1. 检查状态：看 CC_READY (Bit 7) 是否为 1
    BQ76920_Read_Reg(0x00, &status);
    if (!(status & 0x80)) 
    {
        return; // 数据没准备好，直接返回，保持变量里的旧值
    }

    // 2. 使用 Burst 读连续获取高低字节，减少总线占用次数和错位风险
    if (BQ76920_Read_Burst(0x32, raw_data, 2) == 0)
    {
        uint16_t temp =  (uint16_t)(raw_data[0] << 8  | raw_data[1]); 
        new_cc_raw = (int16_t)temp; 
        // 换算成电流 (单位 mA)
        // 公式：ADC * 8.44 / 电阻mΩ
        float current_ma = (float)(new_cc_raw * 8.44f / 5.0f);
        BQ76920_Data.CC = (int16_t)(current_ma);
        
    }

    // 3. 【最关键的一步】清除 CC_READY 标志位
    // 必须要向 Bit 7 写 1 才能清除它 (W1C 逻辑)
    //必须直接写 0x80
    BQ76920_Write_Reg(0x00, 0x80); 
    
}










 /**
 * @brief 获取温度值从而触发保护逻辑
 * 1.读取TS1_HI寄存器和TS1_LO寄存器获取温度值
 * TS1_HI (地址 0x2C)：温度 ADC 值的高 6 位。
 * TS1_LO (地址 0x2D)：温度 ADC 值的低 8 位
 * 2.温度测量共用电压 ADC。你必须确保 SYS_CTRL1 (0x04) 寄存器的 ADC_EN (Bit 4) 位是 1。 注意：如果你已经能读到电压，说明这一位已经开了。
 * 
 * 流程如下：
 * 3.ADC 采样：芯片测得 TS1 引脚的原始数值（14位 ADC），读取TS1_HI寄存器和TS1_LO寄存器
 * 4.换算电压根据公式 将 ADC 值转为电压。公式为：V = ADC * 0.000382f  （单位：V）
 * 5.换算电阻根据公式利用芯片内部的上拉电阻算出 NTC 当前阻值。R = V*1000 / (3.3 - V)
 * 6.查表/计算温度：根据 NTC 的阻值-温度对应表（Datasheet），查出摄氏度。
*/

void BQ76920_Get_Temp(void)
{
    uint8_t hi_byte, lo_byte;//高6位和低8位的温度值
    uint8_t HI,LO;//14位温度值
    int16_t new_temp = 0;//16位温度值（有符号的）
    float temp ;//温度值
    float resistance ;//电阻值
    // 1.读取TS1_HI寄存器和TS1_LO寄存器
        hi_byte = BQ76920_Read_Reg(0x2C, &HI);
        lo_byte = BQ76920_Read_Reg(0x2D, &LO);
        if (hi_byte == 0 && lo_byte == 0)
        {            
        //合并高6位和低八位，得到16位温度值
        new_temp = (uint16_t)((HI << 8) | LO)&0x3FFF ;//高两位无效
        // 3. 换算成电压值
        temp = new_temp * 0.000382f;
       //4. 换算电阻值
       resistance = temp * 10.0 / (3.3 - temp);
       //5. 查表/计算温度
       //根据NTC阻值-温度对应表（Datasheet），查出摄氏度
      if (temp < 3.29) 
      {
    // 计算出的阻值单位是 kOhm (和 103AT 表对应)
    resistance = (10.0f* temp) / (3.3f- temp); 
    BQ76920_Data.Resistance =(uint16_t)resistance;
    // 此时调用查表函数

    BQ76920_Data.Temp = Convert_Resistance_To_Temp(resistance);
      } 

     else
    {
    BQ76920_Data.Temp = -99; // 报错标志：传感器断路
    }
    }
    else
    {
    BQ76920_Data.Temp =-88; // 报错标志：传感器断路
    }
}







/*这个函数会拿着你计算出的电阻，去温度的表里“找位置”。*/


float Convert_Resistance_To_Temp(float r_kohm) {
    // 1. 边界处理
    if (r_kohm >= ntc_table_kohm[0]) return -20.0f; // 电阻太大，说明太冷
    if (r_kohm <= ntc_table_kohm[20]) return 80.0f; // 电阻太小，说明太热

    // 2. 遍历表，寻找电阻所在的区间
    for (int i = 0; i < 20; i++) {
        // 因为 NTC 电阻随温度升高而减小，所以是判断 r 是否在 [大电阻, 小电阻] 之间
        if (r_kohm <= ntc_table_kohm[i] && r_kohm > ntc_table_kohm[i+1]) {
            // 线性插值计算更精确的温度
            // 温度 = 当前起始温度 + (比例 * 步长5度)
            float ratio = (ntc_table_kohm[i] - r_kohm) / (ntc_table_kohm[i] - ntc_table_kohm[i+1]);
            return (-20.0f + (i * 5.0f)) + (ratio * 5.0f);
        }
    }
    return 25.0f; // 默认返回常温
}



/**
 * 计算SOC值
 * 最常用且最可靠的方案是 “安时积分法 (Ah Integration)” 配合 “开路电压 (OCV) 修正
 * 安时积分法的计算公式：公式原型: SOC(t) = SOC(0) - (1 / Q_max) * ∫ I(t) dt
 */

/**
 * 1. 流程闭环示意图
它们是通过 RemainingCharge（电荷量）和 SOC（百分比）这两个变量串联起来的：

阶段一：初始化（开机仅一次）

调用 BQ76920_Get_SOC_From_Voltage。

动作：看一眼现在的电压（比如 3.7V），查表得出是 50%。

关键点：它把 RemainingCharge 设为 1700mAh。这给了系统一个**“初始水位”**。

阶段二：循环运行（每 100ms 一次）

调用 BQ76920_Get_SOC。

动作：测量这 100ms 流过的电流（比如 +100mA）。

关键点：它在 1700mAh 的基础上加一点点，变成 1700.027mAh，再刷新百分比为 50.001%。

作用：只要不关机，它就一直动态追踪电量的细微变化。

阶段三：强力修正（异常或边界触发）

在 BQ76920_Get_SOC 内部。

动作：如果电压已经到了 4.2V（满电保护点），不管积分算到哪了，强行把 RemainingCharge 填满（3400mAh）。

作用：清除安时积分长期运行产生的累积误差
 */


 void BQ76920_Get_SOC(void)
 {
    // 1.计算存入或消耗的电荷，公式：电流量 = 电流(mA) * 时间(h) = mA * (0.1s / 3600s)
    // 电流是CC库仑计的电流值，单位是mA，时间是0.1s，但计算的是mAh，所以要把周期转换为小时，0.1就是100ms，除于3600，得到0.0002778h
    // 如果500ms采样一次，要改成500ms/3600s，0.5s，得到0.0001389h
    // 0.0002778f（对应 1000ms）
    float charge = (float) (BQ76920_Data.CC * 0.0002778f); // 这里假设采样周期是1000ms

    // 2.把得到的电流量加入到剩余电荷中，也可能是减少，充电时增加，放电时减少
    BQ76920_Data.RemainingCharge += charge;

    // 3.控制SOC值在0-100%之间
    // 如果触发了过压保护，强制校准为 100%
    if (BQ76920_Data.MaxVolt > OVER_VOLTAGE_THRESHOLD)
    {
        BQ76920_Data.RemainingCharge = BQ76920_Data.Real_Capacity;
    }
    // 如果触发了过放保护，强制校准为 0%
    else if (BQ76920_Data.MinVolt < UNDER_VOLTAGE_THRESHOLD)
    {
        BQ76920_Data.RemainingCharge = 0.0f;
    }
    // 把soc值限制在0-100%之间
    if (BQ76920_Data.RemainingCharge > BQ76920_Data.Real_Capacity)
    {
        BQ76920_Data.RemainingCharge = BQ76920_Data.Real_Capacity;
    }
     else if (BQ76920_Data.RemainingCharge < 0)
    {
        BQ76920_Data.RemainingCharge = 0;
    }

    // 4.计算SOC值
    BQ76920_Data.SOC = (float)(BQ76920_Data.RemainingCharge / BQ76920_Data.Real_Capacity) * 100.0f;

 }




 /**
 *
 * @brief  通过单体最低电压查找对应的 SOC 百分比 (基于你给的 101 项表),开路电压法
 * @param  volt_mv: 当前电池组中电压最低的单体毫伏值
 * @return float: 对应的 SOC 百分比 (0.0 - 100.0)
 * @note  这个函数会根据你给的电压值，查找对应的 SOC 百分比，
 *        并同步更新剩余 mAh 值。
 */

 void BQ76920_Get_SOC_From_Voltage(float volt_mv)
 {
    // 1.边界检查
    if (volt_mv >SocOcvTab[100])
    {
        BQ76920_Data.SOC = 100.0f;
    }
    else if (volt_mv <SocOcvTab[0])
    {
        BQ76920_Data.SOC = 0.0f;
    }

    // 2.查找SOC值
    for (int i = 0; i < 100; i++)
    {
        if (volt_mv >= SocOcvTab[i] && volt_mv < SocOcvTab[i+1])
        {
           // 计算在该 1% 区间内的偏移比例 (0.0 ~ 1.0)
             float ratio = (volt_mv - (float)SocOcvTab[i]) / (float)(SocOcvTab[i+1] - SocOcvTab[i]);

            // 核心修正：SOC = 索引(整数百分比) + 比例(小数部分)
            BQ76920_Data.SOC = (float)i + ratio;

            /* --- 关键点：同步更新剩余 mAh --- */
            // 只有更新了电荷量，安时积分函数才会基于这个准确的“水位”继续往后算
            BQ76920_Data.RemainingCharge = (BQ76920_Data.SOC / 100.0f) * BAT_CAPACITY_MAH;
            
            // 找到后立刻退出函数，防止被后续循环干扰
            return;
        }
        
    }
}


/**
 * @brief  BMS SOC 初始化校准
 * @note   在 main 函数中、开启 FreeRTOS 任务调度前调用一次
 */
void BQ76920_Init_SOC(void)
{
    // 1. 确保第一次读取有有效电压数据：初始化 ADC、读取增益/偏移并读取一次单体电压
    BQ76920_ADC_Init();
    BQ76920_Get_Offset_Gain();
    BQ76920_Get_Voltage(BQ76920_Data.Cell_V);

    // 2. 初始化剩余电荷,提前找出当前电池组中电压最低的单体毫伏值
    Bms_Find_Voltage_Extremes();

    // 3. 初始化SOC值（使用最小单体电压进行开路电压法估算）
    BQ76920_Get_SOC_From_Voltage(BQ76920_Data.MinVolt);
    // 5. 初始化SOH值
   BQ76920_Data.SOH = 100.0f;
    BQ76920_Data.Real_Capacity = 3400.0f; // 对应 NCR18650B

    // 4. 打印开机信息
   // uart_printf("BQ76920 SOC: %d%%\n",(int)BQ76920_Data.SOC);

}