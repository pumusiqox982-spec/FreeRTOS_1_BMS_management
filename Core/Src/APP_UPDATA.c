#include "APP_UPDATA.h"

// 上位机更新程序状态
APP_UPDATA_State app_updata_state = APP_UPDATA_STATE_WAIT_CMD;

// can 接收缓冲区 最大接收3个数据
CAN_REC_DATA can_rec_data[3] = {0};
// can 还有多少个数据需要处理
uint8_t can_rec_data_cnt = 0;

/**
 * @brief 初始化上位机更新程序
 */
void APP_UPDATA_Init(void)
{
    uart_printf("程序更新初始化\n");
    app_updata_state = APP_UPDATA_STATE_WAIT_CMD;
    // 初始化CAN
    CAN_Init();

    uart_printf("等待上位机更新请求\n");
}


/**
 * @brief 等待上位机更新请求
 */
void APP_UPDATA_Wait_cmd(void)
{
    // 等待上位机更新请求
    CAN_ReceiveData(can_rec_data, &can_rec_data_cnt);
    // 处理接收数据
    for (uint8_t i = 0; i < can_rec_data_cnt; i++)
    {
        // 检查是否是更新请求("sss")
        if (can_rec_data[i].data[0] == 's' && can_rec_data[i].data[1] == 's' && can_rec_data[i].data[2] == 's')
        {
            // 处理更新请求
            app_updata_state = APP_UPDATA_STATE_SEND_APP;
        }
    }
}


/**
 * @brief 发送更新程序
 */
void APP_UPDATA_Send_app(void)
{
    uart_printf("发送更新程序\n");
    
}


/**
 * @brief 循环程序
 * 
 */
void APP_UPDATA_Work(void)
{
    switch (app_updata_state)
    {
    case APP_UPDATA_STATE_WAIT_CMD:
        APP_UPDATA_Wait_cmd();
        break;
    case APP_UPDATA_STATE_SEND_APP:
        APP_UPDATA_Send_app();
        break;
    default:
        break;
    }
}
