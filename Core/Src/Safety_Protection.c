#include "Safety_Protection.h"

/*需要的头文件*/
#include "Get_and_Convert.h"
#include "main.h"


// 全局变量
// 全局数据实例，供其他文件通过 extern 使用
Safety_Status Safety_Data = {0}; //分配内存，要是只声明不给赋值，会报错，在这里相当于是给内存赋值为0
Watchdog_Monitor_t Watchdog_Monitor_Data = {0}; // 监控数据结构体实例


// 手动模式
uint8_t Manual_Mode_Active;   // 1: 手动模式激活，自动保护/均衡暂停
uint32_t Manual_Mode_Timer;   // 手动模式计时器（毫秒）





/******************************用于保护逻辑的函数******************************** */






/** 
 * 如果任意一节电压 > 4.2V（过压 OV），触发过压保护，关闭充电 MOSFET。
 * 如果任意一节电压 < 2.7V（过放 UV），触发过放保护，关闭放电 MOSFET。
 * 如果电流 < -18A（过流），触发过流保护，关闭放电放电 MOSFET。
 * 如果电流 > 18A（过流），触发过流保护，关闭充电 MOSFET。
 * 如果温度 > 60℃（过温），触发过温保护，关闭充电 MOSFET。
 * 如果温度 < -20℃（过冷），触发过冷保护，关闭放电 MOSFET。
 * 如果温度 > 50℃（过温），触发过温报警。
 * 如果温度 < 0℃（过冷），触发过冷报警。
*/
void Check_Safety_protection(void)
{
    // --- 0. 优先级最高：检查硬件寄存器状态 ---
    uint8_t sys_stat = 0; 
    BQ76920_Read_Reg(0x00, &sys_stat); 
    // 如果硬件检测到：过流(OCD)、短路(SCD)、过压(OV)、欠压(UV)
    // 0x3E 包含了这四种保护以及 DEVICE_XREADY
    if (sys_stat & 0x3E) 
    {
        if (Safety_Data.Lock_Flag == 0) // 仅在第一次触发时打印，避免死循环打印
        {
            Safety_Data.Lock_Flag = 1; 
            uart_printf("检测到硬件报警(0x%02X)！系统已强制进入安全锁定模式。\n", sys_stat);
        }
    }

    // --- 1. 如果处于锁定状态 (Lock_Flag == 1)，软件逻辑失效 ---
    if (Safety_Data.Lock_Flag == 1)
    {
        // 强制切断所有 MOSFET，保护电路
        Bms_Turn_Off_Charge_MOS();
        Bms_Turn_Off_Discharge_MOS();
        return; // 直接退出函数，不再运行下方的软件采样和判断逻辑
    }

    // --- 2. 手动模式下，跳过软件保护逻辑（硬件保护仍在，且硬件锁定已处理）---
    if (Manual_Mode_Active)
    {
        // 注意：锁定标志已经在上面判断过，这里如果未锁定则不做任何改变
        // 即保持手动控制后的MOS状态不变
        return;
    }




    // 1. 先获取数据
    Bms_Find_Voltage_Extremes(); 
    BQ76920_Get_CC();
    BQ76920_Get_Temp();
    // 定义临时决策变量（逻辑上的“允许位”）
    // 默认保持现状，只有明确触发保护才清零，明确满足恢复才置位
    static uint8_t chg_allowed = 1; 
    static uint8_t dsg_allowed = 1;
    // --- 2. 保护检查 (优先级最高：关断逻辑) ---
    
    // 过压保护
    if (BQ76920_Data.MaxVolt > OVER_VOLTAGE_THRESHOLD)
    {
        chg_allowed = 0;
    }
    
    // 过放保护
    if (BQ76920_Data.MinVolt < UNDER_VOLTAGE_THRESHOLD) 
    {
        dsg_allowed = 0;
    }

    // 严重故障：过流、极端高低温 -> 触发 Lock_Flag
    if (BQ76920_Data.CC < CURRENT_THRESHOLD || 
        BQ76920_Data.CC > CHARGE_CURRENT_THRESHOLD ||
        BQ76920_Data.Temp > OVER_TEMP_THRESHOLD ||
        BQ76920_Data.Temp < UNDER_TEMP_THRESHOLD)
    {
        Safety_Data.Lock_Flag = 1; // 触发永久锁定
        uart_printf("严重故障！系统锁定！\n");
    }
    // --- 3. 恢复检查 (只有在非锁定状态下才判断) ---
    if (Safety_Data.Lock_Flag == 0)
    {
        // 过压恢复：必须降到恢复阈值以下
        if (BQ76920_Data.MaxVolt < OV_RECOVERY_THRESHOLD && BQ76920_Data.MinVolt > UV_RECOVERY_THRESHOLD&&BQ76920_Data.Temp < OVER_TEMP_RECOVERY_THRESHOLD && BQ76920_Data.Temp > UNDER_TEMP_RECOVERY_THRESHOLD)
        {
            chg_allowed = 1;
            dsg_allowed = 1;
        }
        
    }
    else 
    {
        // 只要处于 Lock_Flag 锁定状态，强制禁止所有路径
        chg_allowed = 0;
        dsg_allowed = 0;
    }

    // 根据刚才所有逻辑判断出来的“信号灯”状态，一次性操作寄存器
    if (chg_allowed == 1) {
        Bms_Turn_On_Charge_MOS();
    } else {
        Bms_Turn_Off_Charge_MOS();
    }

    if (dsg_allowed == 1) {
        Bms_Turn_On_Discharge_MOS();
    } else {
        Bms_Turn_Off_Discharge_MOS();
    }


} 






/**
 * @brief 关闭充电 MOSFET
 */
void Bms_Turn_Off_Charge_MOS(void)
{
    uint8_t reg_val = 0;

    // 1. 读取当前寄存器值
    if (BQ76920_Read_Reg(0x05, &reg_val) != 0) return;

    // 2. 逻辑修改：
    // reg_val &= 0xFE; // 清除第 0 位 (CHG)
    // reg_val |= 0x40; // 【核心】强制将第 6 位 (CC_EN) 置 1，确保它永远不会被关闭
    
    reg_val = (reg_val & 0xFE) | 0x40; 

    // 3. 写回寄存器
    BQ76920_Write_Reg(0x05, reg_val);
}

/**
 * @brief 开启充电 MOSFET
 */
void Bms_Turn_On_Charge_MOS(void)
{
    uint8_t reg_val = 0;

    if (BQ76920_Read_Reg(0x05, &reg_val) != 0) return;

    // 2. 逻辑修改：
    // reg_val |= 0x01; // 开启第 0 位 (CHG)
    // reg_val |= 0x40; // 【核心】强制确保 CC_EN 开启
    
    reg_val |= 0x41; // 同时置位 CHG 和 CC_EN

    BQ76920_Write_Reg(0x05, reg_val);
}



/**
 * @brief 关闭放电 MOSFET
 */
void Bms_Turn_Off_Discharge_MOS(void)
{
    uint8_t reg_val = 0;
    if (BQ76920_Read_Reg(0x05, &reg_val) != 0) return;

    // 核心修正：清除 Bit 1，同时【强制】确保 Bit 6 为 1
    reg_val = (reg_val & 0xFD) | 0x40; 

    BQ76920_Write_Reg(0x05, reg_val);
}

/**
 * @brief 开启放电 MOSFET
 */
void Bms_Turn_On_Discharge_MOS(void)
{
    uint8_t reg_val = 0;
    if (BQ76920_Read_Reg(0x05, &reg_val) != 0) return;

    // 核心修正：置位 Bit 1，同时【强制】确保 Bit 6 为 1
    reg_val |= 0x42; // 0x40 (CC_EN) | 0x02 (DSG)

    BQ76920_Write_Reg(0x05, reg_val);
}





/**
 * @brief 带安全检查的手动解除锁定
 * @note  此函数通常由按键长按、上位机指令或系统自检触发
 */
void Bms_Reset_Safety_Lock(void)
{
    // 1. 获取最新实时数据（确保判断的是当下，而不是一分钟前的旧数据）
    Bms_Find_Voltage_Extremes(); 
    BQ76920_Get_CC();
    BQ76920_Get_Temp();

    // 2. 检查物理环境是否真正回到了“安全恢复区间”
    // 注意：这里必须全部使用 RECOVERY（恢复）阈值，而不是 PROTECTION（保护）阈值
    uint8_t is_volt_ok = (BQ76920_Data.MaxVolt < OV_RECOVERY_THRESHOLD) && 
                         (BQ76920_Data.MinVolt > UV_RECOVERY_THRESHOLD);
                         
    uint8_t is_temp_ok = (BQ76920_Data.Temp < OVER_TEMP_RECOVERY_THRESHOLD) && 
                         (BQ76920_Data.Temp > UNDER_TEMP_RECOVERY_THRESHOLD);
                         
    // 电流检查：必须接近0（防止带负载直接强行解锁导致打火）
    // 这里的 500mA 是一个经验值，你可以根据实际检流电阻精度调整
    uint8_t is_curr_ok = (BQ76920_Data.CC < 500) && (BQ76920_Data.CC > -500);

    // 3. 执行复位决策
    if (is_volt_ok && is_temp_ok && is_curr_ok)
    {
        // --- 核心动作 1：清除硬件状态寄存器 
        uint8_t current_stat = 0;
        BQ76920_Read_Reg(0x00, &current_stat);
        // 读取到哪些故障，就写回哪些 1，告诉芯片“我知道了，请复位”
        BQ76920_Write_Reg(0x00, current_stat); 

        // --- 核心动作 2：解除软件锁定标志 ---
        Safety_Data.Lock_Flag = 0; 
        
        uart_printf("System: [解锁成功] 硬件故障位已清除，系统恢复软件监控。\n");
    
    }
    else
    {
        // 4. 细分报错原因，方便调试（通过串口告诉你到底哪里还没达标）
        uart_printf("System: [拒绝复位] 条件未满足: ");
        if (!is_volt_ok) uart_printf("电压超限(%d mV); ", BQ76920_Data.MaxVolt);
        if (!is_temp_ok) uart_printf("温度异常(%d C); ", BQ76920_Data.Temp);
        if (!is_curr_ok) uart_printf("电流过大(%d mA); ", BQ76920_Data.CC);
        uart_printf("\n");
    }
}



/**
 * @brief 开启均衡模式，被动均衡
 * 
 * 1.寄存器地址
 *  这是你写代码最需要关注的地方。在手册的 Register Map 章节：
 *  寄存器名称：CELLBAL1
 *  地址：0x01
 *  功能：这个寄存器的低 5 位（CB1-CB5）分别控制 5 节电芯的均衡开关。
 *  操作：向对应的位写 1 开启内部均衡 FET，写 0 关闭。
 *  注意：开启均衡模式后，相邻芯片绝对不能开启均衡
 *  每秒调用一次，建议放在 1000ms 级别的任务中
 */



 void Bms_Turn_On_Balance_Mode(void)
 {
    // 手动模式下，不自动调节均衡（但允许上位机直接写寄存器）
    if (Manual_Mode_Active)
    {
        // 注意：上位机已经通过命令写入了0x01寄存器，这里不做任何修改
        return;
    }

    // 以下为原来的自动均衡逻辑
    static uint32_t balance_timer = 0;
    static uint8_t balance_phase = 0;
    uint8_t balance_command = 0;

    if (BQ76920_Data.CC < 100 || BQ76920_Data.MaxVolt < 3500)
    {
        BQ76920_Write_Reg(0x01, 0x00);
        return;
    }
    if (BQ76920_Data.DiffVolt < 50) 
    {
        BQ76920_Write_Reg(0x01, 0x00);
        return;
    }

    balance_timer++;
    if (balance_timer % 30 == 0) balance_phase = !balance_phase;

    for (int i = 0; i < 5; i++) 
    {
        if ((BQ76920_Data.Cell_V[i] - BQ76920_Data.MinVolt) > 50) 
        {
            if ((balance_phase == 0 && i % 2 == 0) || (balance_phase == 1 && i % 2 != 0)) 
                balance_command |= (1 << i);
        }
    }
    if (balance_timer % 10 == 0) balance_command = 0x00;
    BQ76920_Write_Reg(0x01, balance_command);
 }
 




 /**
  * 硬件保护电路
  * 由于前面都是软件保护，但是代码是几百毫秒才反应过来，所以这里开启硬件保护，当检测到意外会以微秒速度关闭MOSET。
  * 1.分别需要实现的保护：PROTECT1 (地址 0x06)：控制短路保护 (SCD)
                      PROTECT2 (地址 0x07)：控制过流保护 (OCD)
                      PROTECT3 (地址 0x08)：控制硬件欠压 (UV) 和过压 (OV) 的延迟
                      SYS_CTRL2 (地址 0x05)：控制充电/放电 MOS 管的开关（CHG_ON, DSG_ON）
                      SYS_STAT (地址 0x00)：硬件状态标志位（查看是哪个保护触发了，以及 ALERT 引脚的状态）
  * 
  * 2. 关键寄存器位定义及功能
    A. PROTECT1 (0x06) - 短路保护 (SCD)
    这个寄存器决定了当发生短路（电流巨大）时，芯片在多少微秒内切断输出。
    RSNS (Bit 7)：决定输入范围。通常设为 1，表示更宽的范围。
    SCD_THRESH (Bits 2-0)：设定短路电流阈值。
    SCD_DELAY (Bits 4-3)：设定延迟时间（如 70μs, 100μs, 200μs, 400μs）。
    B. PROTECT2 (0x07) - 过流保护 (OCD)
    这个用于防止普通的大电流过载。
    OCD_THRESH (Bits 3-0)：设定过流电流阈值。
    OCD_DELAY (Bits 6-4)：设定延迟时间（从 8ms 到 1.2s 不等）。
    C. SYS_STAT (0x00) - 保护触发后的“案发现场”
    当你的 ALERT 引脚变高时，你需要读这个寄存器来判断原因：
    OCD (Bit 4)：1 表示触发了过流保护。
    SCD (Bit 3)：1 表示触发了短路保护。
    OV (Bit 2)：1 表示触发了过压保护。
    UV (Bit 1)：1 表示触发了欠压保护。
    重要： 触发后，你必须向该位 写 1 才能清除错误状态并允许重新开启 MOS 管。
    D. PROTECT3 (0x08) - 电压保护的“延时管家”
    如果说前面两个寄存器是设置“多高电压”算危险，那么 PROTECT3 就是设置“持续多久”才动手。它决定了芯片在发现电压异常后，是立即“掐断”电路，还是先观察一会儿。
    UV_DELAY (Bit 7-4)：欠压保护延迟
    功能：当某节电芯电压低于你设置的 UV 阈值时，芯片会开始计时。只有异常状态持续时间超过这个值，才会触发报警（ALERT 引脚拉高）并关断放电 MOS 管。
    意义：非常关键！它可以防止电机启动瞬间的“压降（Voltage Sag）”导致 BMS 误触发欠压保护。
    OV_DELAY (Bit 3-0)：过压保护延迟
    功能：当电压高于 OV 阈值时，持续该时间后触发报警并关断充电 MOS 管。
    意义：防止由于充电器电压波动或 ADC 采样噪声引起的频繁停充。

    工作流程：
  * 3. 需要先读取出GAIN和OFFEST，通过转换数值存进寄存器里面作为阈值，不可以直接输入数值
  *    过压（OV）4220mV，欠压（UV）2500mV，过流（OCD）18A，短路（SCD）30A，过温（OT）60℃，过冷（UT）-20℃
  * 从对应的寄存器中读取 [ADCGAIN] 和 [ADCOFFSET] 的值。请注意，ADCGAIN 的单位为 μV/LSB，而 ADCOFFSET 的单位为 mV。
  * a. OV_TRIP_FULL = (OV – ADCOFFSET) ÷ ADCGAIN b. UV_TRIP_FULL = (UV – ADCOFFSET) ÷ ADCGAIN
  * 从完整的 14 位值中移除最高有效位 (MSB) 的 2 位和最低有效位 (LSB) 的 4 位，仅保留中间的 8 位。这可以通过将 OV_TRIP_FULL 和 UV_TRIP_FULL 二进制值右移 4 位并移除最高有效位 (MSB) 的 2 位来实现。
  * 将OV_TRIP 和 UV_TRIP 分别写入相应的寄存器。地址分别为0x09和0x0A
  * 4. 设置过压和欠压的延迟时间，意思是说当检测到过压或欠压时，必须持续超过设定的时间才会真正触发保护，这样可以避免瞬间的电压波动导致误触发。
  * 5. 查看保护文档吧，懒得在这写了。
  * 
  */



  void BQ76920_Hardware_Protection_Init(void)
  {
     // 1.先前写过获取GAIN和OFFSET的函数了，这里直接用就行了
     // 公式：OV_TRIP_FULL = (OV – ADCOFFSET) ÷ ADCGAIN
    // 注意：手册说 ADCGAIN 单位是 μV/LSB，所以电压要换算成 μV，或者 OFFSET 换算成 mV
     BQ76920_Get_Offset_Gain();
     uint16_t ov_trip_full = (uint16_t)((OVER_VOLTAGE_THRESHOLD * 1000 - BQ76920_Data.OFFSET*1000 ) / BQ76920_Data.GAIN);
     uint16_t uv_trip_full = (uint16_t)((UNDER_VOLTAGE_THRESHOLD * 1000 - BQ76920_Data.OFFSET*1000 ) / BQ76920_Data.GAIN);

     // 2.把计算出来的值转换成寄存器需要的格式（去掉最高两位和最低四位）
    uint8_t ov_reg_value = (uint8_t)(ov_trip_full >> 4);//这样做强转只保留了八位，右边移动四位去掉低四位，最后强转去除高两位
    uint8_t uv_reg_value = (uint8_t)(uv_trip_full >> 4);//这样做强转只保留了八位，右边移动四位去掉低四位，最后强转去除高两位

     // 3.写入寄存器，这样就设定好了过压和欠压的阈值了
     BQ76920_Write_Reg(0x09, ov_reg_value);
     BQ76920_Write_Reg(0x0A, uv_reg_value);

     /* --- 稳健型硬件电流保护设置 --- */

    // 4. 配置 PROTECT1 (SCD): 
    // RSNS=0, 100us延时(略微防抖), 22mV阈值 (4.4A)
    // 二进制: 0 0 0 01 000 -> 0x08
    BQ76920_Write_Reg(0x06, 0x08);

    // 5. 配置 PROTECT2 (OCD):
    // 20ms延时(避开瞬态), 11mV阈值 (2.2A)
    // 二进制: 0 001 0001 -> 0x11
    BQ76920_Write_Reg(0x07, 0x11);

    // 6. 配置 PROTECT3 (电压延时): 
    // UV 延时 4s (放宽), OV 延时 1s (收紧)
    // 二进制: 01 00 0000 -> 0x40 (注意：低4位是RSVD)
    BQ76920_Write_Reg(0x08, 0x40);

    // 7. 开启总闸
    // 写入 0x03 开启 CHG 和 DSG MOS管，且确保 DELAY_DIS=0，这里暂时不用开启，初始化那里开启了
    uint8_t reg_val = 0;
     // 1. 必须判断读取是否成功
    if (BQ76920_Read_Reg(0x05, &reg_val) == 0) 
    {
    // 如果读取失败，千万不要随便写回！可以打印报错
    uart_printf("Error: Read 0x05 Failed!\n");
    }
  }






  /** * @brief  读取并打印 BQ76920 的硬件故障状态
 * @return 返回状态寄存器的原始值，方便逻辑判断
 */
uint8_t BQ76920_Diagnose_Fault(void)
{
    uint8_t stat = 0;
    
    // 1. 从寄存器 0x00 读取当前状态
    if (BQ76920_Read_Reg(0x00, &stat) != 0) {
        uart_printf("Error: I2C 通信失败，无法读取状态！\n");
        return 0;
    }

    // 检查是否有任何故障位 (Bit 5 ~ Bit 0)
    // 如果这些位全是 0，说明物理环境非常安全
    if ((stat & 0x3F) == 0) {
     //   uart_printf("状态: [安全] (SYS_STAT: 0x%02X)\n", stat);
        return 0; 
    }

    // 3. 逐位解析打印 (Bit 5 到 Bit 1)
    uart_printf("--- [硬件报警触发] --- (寄存器值: 0x%02X)\n", stat);
    
    if (stat & 0x20) uart_printf("  [!] DEVICE_XREADY: 芯片内部故障/采样未准备好\n");
    if (stat & 0x10) uart_printf("  [!] OVRD_ALERT: 外部覆盖/ALERT引脚强制触发\n");
    if (stat & 0x08) uart_printf("  [!] UV: 欠压保护 (电池电压过低)\n");
    if (stat & 0x04) uart_printf("  [!] OV: 过压保护 (电池电压过高)\n");
    if (stat & 0x02) uart_printf("  [!] SCD: 短路保护 (电流极大！)\n");
    if (stat & 0x01) uart_printf("  [!] OCD: 过流保护 (负载过重)\n");
    
    uart_printf("----------------------\n");

    return stat;
}





/**
 * 计算电池SOH状态
 * 我们采用的是 “片段积分法”。只要满足一定的条件，我们就能更新 SOH。
 * 详细：假设电池没老（SOH=100%），电量从 80% 掉到 30%（50% 电量），理论上应该流出 3400*50% = 1700mAh 的电量。
 * 如果你实测流出了 1700mAh 的电量，但 SOC  却从 80% 掉到了 20%，说明电池缩水了
 * 1.开始记录：当电池掉到80%的时候开始记录， SOH_SOC_Start=80%,并且开始记录流出的电量SOH_mAh_Sum
 * 2.累加过程：如果正在记录，就不断累加当前的安时积分
 * 3. 结束记录并计算：当 SOC 跨度超过 30%
 */



 void BQ76920_GET_SOH(void)
 {
   // 1. 启动记录：当电池处于平稳放电，且 SOC 较高时启动
    // 我们设定从 SOC > 90% 且开始放电时触发
    if (BQ76920_Data.SOH_Record_Flag == 0)
    {
        if (BQ76920_Data.SOC > 80.0f && BQ76920_Data.CC < -100) // 电流<-100mA认为在稳定放电
        {
            BQ76920_Data.SOH_SOC_Start = BQ76920_Data.SOC;
            BQ76920_Data.SOH_mAh_Sum = 0;
            BQ76920_Data.SOH_Record_Flag = 1;
            uart_printf("[SOH] 开始片段记录, 起始SOC: %.1f%%\r\n", BQ76920_Data.SOH_SOC_Start);
        }
    }
    // 2.累加过程：如果正在记录，就不断累加当前的安时积分
    if (BQ76920_Data.SOH_Record_Flag == 1)
    {
      // 只有在放电时累加 (CC为负，所以取负号变正电量)
        if (BQ76920_Data.CC < 0)
        {
            // 这里的时间常数要和 task3 周期对应，假设是 1000ms
            float delta_q = (float)(-BQ76920_Data.CC * 0.0002778f); 
            BQ76920_Data.SOH_mAh_Sum += delta_q;
        }
        // 3. 结束记录并计算：当 SOC 跨度超过 30%
        float soc_diff = BQ76920_Data.SOH_SOC_Start - BQ76920_Data.SOC;
        if (soc_diff >= 30.0f)
        {
            // 核心计算：实际容量 = 累计电量 / SOC变化比例
            float measured_real_cap = BQ76920_Data.SOH_mAh_Sum / (soc_diff / 100.0f); 
            
            // 计算新的 SOH
            float new_soh = (measured_real_cap / BAT_CAPACITY_MAH) * 100.0f;

            // 限制范围，防止计算错误导致 SOH 乱飞
            if (new_soh > 105.0f) new_soh = 100.0f;
            if (new_soh < 50.0f)  new_soh = 50.0f;

            // 平滑滤波：不要一次性改完，防止单次噪声干扰
            BQ76920_Data.SOH = (BQ76920_Data.SOH * 0.8f) + (new_soh * 0.2f); //* 0.8 代表“信任过去”： 保持 80% 的旧数据不动，维持稳定性。0.2 代表“拥抱未来”： 只吸纳 20% 的新测量结果，慢慢修正。
            
            // 重要：同步更新实际总容量，这样 SOC 积分的分母就变准了
            BQ76920_Data.Real_Capacity = (BQ76920_Data.SOH / 100.0f) * BAT_CAPACITY_MAH;

            uart_printf("[SOH] 片段计算完成! 测得容量: %dmAh, 更新SOH: %d%%\r\n", 
                (uint16_t)measured_real_cap, (uint8_t)BQ76920_Data.SOH);

            // 复位记录器，等待下一次循环
            BQ76920_Data.SOH_Record_Flag = 0;
        }
        
        // 异常退出：如果突然开始大电流充电，本次片段记录失效
        if (BQ76920_Data.CC > 50) 
        {
            BQ76920_Data.SOH_Record_Flag = 0;
            uart_printf("[SOH] 检测到充电，中止本次片段记录。\r\n");
        }
    }
 }
    

 