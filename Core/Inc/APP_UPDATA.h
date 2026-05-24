#ifndef __APP_UPDATA_H
#define __APP_UPDATA_H

#include "main.h"
#include "int_can.h"



#define APP_UPDATA_CMD "sss"

typedef enum
{
    APP_UPDATA_STATE_WAIT_CMD = 0,
    APP_UPDATA_STATE_SEND_APP,
} APP_UPDATA_State;







/**
 * @brief 初始化上位机更新程序
 */
void APP_UPDATA_Init(void);


/**
 * @brief 等待上位机更新请求
 */
void APP_UPDATA_Wait_cmd(void);


/**
 * @brief 发送更新程序
 */
void APP_UPDATA_Send_app(void);


/**
 * @brief 循环程序
 * 
 */
void APP_UPDATA_Work(void);



#endif
