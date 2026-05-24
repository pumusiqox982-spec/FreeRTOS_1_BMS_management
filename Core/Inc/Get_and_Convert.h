#ifndef __GET_AND_CONVERT_H__
#define __GET_AND_CONVERT_H__




#include "iic_soft.h"


typedef struct {
    uint16_t Cell_V[5];   // 存储5节电芯电压
    uint16_t MaxVolt;        // 最大电压值
    uint8_t MaxIndex;     // 最大电压对应的单体编号
    uint16_t MinVolt;        // 最小电压值
    uint8_t MinIndex;     // 最小电压对应的单体编号
    uint16_t DiffVolt;       // 压差            
    int16_t CC;             // 电流值
    float  GAIN;             // 增益值
    int8_t OFFSET;           // 偏移量
    uint16_t Temp;             // 温度值
    uint16_t Resistance;       // NTC温度电阻值
    float RemainingCharge;    // 剩余电荷
    float SOC;                // SOC值

    float SOH;              // 健康度 (0~100.0)
    float Real_Capacity;    // 当前实际可用容量 (mAh)
    float SOH_SOC_Start;    // 记录开始时的 SOC
    float SOH_mAh_Sum;      // 记录这段时间内积攒的电量
    uint8_t SOH_Record_Flag;// 记录状态：0-空闲，1-正在记录
}g_bq_status;

extern g_bq_status BQ76920_Data;





// 定义 NTC 阻值表：从 -20℃ 到 80℃，步进为 5℃
// 单位：kOhm (千欧)
static const float ntc_table_kohm[] = {
    97.07, 72.50, 54.66, 41.61, 31.97, // -20, -15, -10, -5, 0 ℃
    24.79, 19.39, 15.30, 12.18, 9.77,  // 5, 10, 15, 20, 25 ℃
    7.88,  6.40,  5.22,  4.29,  3.54,  // 30, 35, 40, 45, 50 ℃
    2.94,  2.45,  2.05,  1.73,  1.46,  // 55, 60, 65, 70, 75 ℃
    1.24                               // 80 ℃
};



// 根据 NCR18650B 典型值
#define BAT_CAPACITY_MAH  3400.0f  // 额定容量 3400mAh
#define SAMPLE_INTERVAL_S   0.1f     // 你的 Task 周期是 100ms
#define MAX_VOLT_100        4200     // 4.2V 对应 100%
#define MIN_VOLT_0          3200 // 3.2V 对应 0% (NCR 系列放电深度深)





/*封装好的函数*/

void BQ76920_Get_Voltage(uint16_t *pVoltage);  //获取电压值
void BQ76920_Get_Offset_Gain(void);     //获取偏移量和增益
void Bms_Find_Voltage_Extremes(void); // 查找最大电压和最小电压
void BQ76920_Get_CC(void); // 获取电流值
float Convert_Resistance_To_Temp(float r_kohm); // 转换电阻值为温度
void BQ76920_Get_Temp(void); // 获取温度值
void BQ76920_Get_SOC_From_Voltage(float volt_mv); // 从最小电压获取SOC值
void BQ76920_Get_SOC(void); // 从电荷值获取SOC值
void BQ76920_Init_SOC(void); // 初始化SOC值



#endif
