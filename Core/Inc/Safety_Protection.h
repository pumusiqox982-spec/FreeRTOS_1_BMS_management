#ifndef __SAF_PROTECTION_H__
#define __SAF_PROTECTION_H__

#include "main.h"


// 手动模式控制
extern uint8_t Manual_Mode_Active;   // 1: 手动模式激活，自动保护/均衡暂停
extern uint32_t Manual_Mode_Timer;   // 手动模式计时器（毫秒）
#define MANUAL_MODE_TIMEOUT_MS  30000  // 30秒后自动退出手动模式




typedef struct {
        uint16_t Over_Voltage_Threshold;            //过压保护的阈值
        uint16_t Under_Voltage_Threshold;           //过放保护的阈值
        int16_t  Current_Threshold;                   //过流保护的阈值                          
        uint8_t Lock_Flag;                            //锁定标志位
        uint8_t OVER_TEMP_THRESHOLD;                  //过温保护的阈值
        uint8_t UNDER_TEMP_THRESHOLD;                 //过冷保护的阈值
        uint8_t OVER_TEMP_ALARM_THRESHOLD;            //过温报警阈值
        uint8_t UNDER_TEMP_ALARM_THRESHOLD;           //过冷报警阈值
                                                      
   
}Safety_Status;

extern Safety_Status Safety_Data;

typedef struct {
    uint8_t Task2_RunFlag; // 任务2（均衡/打印）运行标志
    uint8_t Task3_RunFlag; // 任务3（采样/保护）运行标志
} Watchdog_Monitor_t;

extern Watchdog_Monitor_t Watchdog_Monitor_Data;









/*定义数值阈值*/
#define OVER_VOLTAGE_THRESHOLD 4220 // 把过压保护的阈值设置为4.2V
#define UNDER_VOLTAGE_THRESHOLD 2500 // 把过放保护的阈值设置为2.5V
#define OV_RECOVERY_THRESHOLD 4000 // 过压保护恢复阈值设置为4.0V
#define UV_RECOVERY_THRESHOLD 3500 // 过放保护恢复阈值设置为3.5V
#define CURRENT_THRESHOLD -18500 // 过流保护阈值设置为-18.5A
#define CURRENT_ALARM_THRESHOLD -15000 // 过流报警阈值设置为-15A
#define CHARGE_CURRENT_THRESHOLD 18500 // 充电电流阈值设置为18.5A
#define CHARGE_CURRENT_ALARM_THRESHOLD 15000 // 充电电流报警阈值设置为15A
#define LOCK_FLAG 0 // 锁定标志位为0
#define OVER_TEMP_THRESHOLD 60 // 过温保护阈值设置为60℃
#define OVER_TEMP_RECOVERY_THRESHOLD 30 // 过温恢复阈值设置为30℃
#define UNDER_TEMP_THRESHOLD -20 // 过冷保护阈值设置为-20℃
#define UNDER_TEMP_RECOVERY_THRESHOLD -10 // 过冷恢复阈值设置为-10℃
#define OVER_TEMP_ALARM_THRESHOLD 50 // 过温报警阈值设置为50℃
#define UNDER_TEMP_ALARM_THRESHOLD 0 // 过冷报警阈值设置为0℃



//封装好的函数，可以直接使用
void Bms_Turn_Off_Charge_MOS(void); // 关闭充电 MOSFET
void Bms_Turn_On_Charge_MOS(void); // 开启充电 MOSFET
void Bms_Turn_On_Discharge_MOS(void); // 开启放电 MOSFET
void Bms_Turn_Off_Discharge_MOS(void); // 关闭放电 MOSFET
void Check_Safety_protection(void); // 检查安全保护
 void Bms_Turn_On_Balance_Mode(void); // 开启均衡模式
 void BQ76920_Hardware_Protection_Init(void); // 初始化硬件保护
 uint8_t BQ76920_Diagnose_Fault(void); // 诊断故障，返回故障状态
  void BQ76920_GET_SOH(void); // 获取SOH值
  void Bms_Reset_Safety_Lock(void); // 重置安全锁
  void BQ76920_Hardware_Protection_Init(void); // 初始化硬件保护
 





#endif
