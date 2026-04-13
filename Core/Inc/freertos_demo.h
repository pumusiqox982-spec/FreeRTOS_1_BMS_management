#ifndef __FREERTOS_DEMO_H__
#define __FREERTOS_DEMO_H__

/*需要用到的头文件*/
#include "iic_soft.h"



void freertos_start(void);
int uart_printf(const char* format, ...);
void BQ76920_ADC_Init(void);
void uart_send(const char *str, int len);
/**
    @brief: 任务1；初始化bq76920的ADC，必须先开启ADC
    SYS_CTRL1 寄存器中的[ADC_EN](第四位)位必须设置为1，
    VC1 至 VC5 的测量分别在 50 毫秒的抽取周期内进行，此时 CELLBAL1 寄存器中的所有位均为 0；
    在 12.5 毫秒的抽取周期内进行，此时 CELLBAL1 寄存器中的任何位为 1。具体详情查看手册
*/




#endif /* __FREERTOS_DEMO_H__ */
