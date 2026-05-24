#include "int_can.h"




/*需要用到的全局变量*/
extern CAN_HandleTypeDef hcan;



/**
 * @brief can通信初始化
 * 需要配置过滤器 can要手动开启（因为默认是休眠状态）
 */
void CAN_Init(void)
{
   CAN_FilterTypeDef filterConfig = {0};
   filterConfig.FilterBank = 0;                           // 选择过滤器0
   filterConfig.FilterMode = CAN_FILTERMODE_IDMASK;       // 过滤器模式：掩码模式
    filterConfig.FilterScale = CAN_FILTERSCALE_32BIT;       // 过滤器选择的位数：32位

   //填下寄存器  （现在作为上位机 接受A程序使用的can id 为0）
   filterConfig.FilterIdHigh = 0x0000;                   // 过滤器ID
   filterConfig.FilterIdLow = 0x0000;
   
   // 现在作为上位机 接受A程序使用的can id 为0  这样就是只接收id为0的消息
    filterConfig.FilterMaskIdHigh = 0xFFe0;
    filterConfig.FilterMaskIdLow = 0x0000;

   // 使用哪个队列
    filterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;    // 有三个FIFO，0、1、2
    // 激活过滤器
    filterConfig.FilterActivation = CAN_FILTER_ENABLE;     

    // 1.配置过滤器
    HAL_CAN_ConfigFilter(&hcan, &filterConfig);

    // 2.开启can通信
    HAL_CAN_Start(&hcan);
}



/**
 * @brief can发送数据
 * @param id 发送的ID
 * @param data 要发送的数据 
 * @param len 数据长度 (最长8字节)
 */
void CAN_SendData(uint16_t id, uint8_t *data, uint8_t len)
{
    // 等待发送邮箱空闲
    while(HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0); // 获取空闲的发送邮箱数量，返回值为0、1、2，表示有多少个发送邮箱当前是空闲的。如果返回值为0，表示所有发送邮箱都被占用，需要等待。
    

    CAN_TxHeaderTypeDef txHeader = {0};
    txHeader.StdId = id;              // 标准ID
    txHeader.IDE = CAN_ID_STD;        // 标准格式
    txHeader.RTR = CAN_RTR_DATA;      // 数据帧
    txHeader.DLC  = len;               // 数据长度


    uint32_t mailbox; // 邮箱变量
    // 将发送的东西添加到邮箱0    &mailbox指向一个无符号32位变量的指针，函数成功执行后会将实际分配到的发送邮箱编号（0, 1, 或 2）存入这个变量
    HAL_CAN_AddTxMessage(&hcan, &txHeader, data, &mailbox);  //data	const uint8_t[]	指向要发送的数据缓冲区（数组）。注意发送的最大长度由 txHeader.DLC 决定，不能超过8字节。
}



/**
 * @brief can接收数据
 * @param rec_data 接收数据的结构体指针
 * @param data_count 用于 返回 CAN 接收 FIFO 中当前待读取的消息 条数
 */
void CAN_ReceiveData(CAN_REC_DATA *rec_data,uint8_t *data_count)
{
    *data_count = HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0); //*data_count 表示“data_count 指针所指向的那个内存位置”。把获得的数值存放到这个位置，就相当于修改了调用者传递进来的那个变量。
    for (uint8_t i = 0; i < *data_count; i++)
    {
        // 清空数据
        memset(&rec_data[i], 0, sizeof(CAN_REC_DATA));
        HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &rec_data[i].RxHeader, rec_data[i].data);  
    }
    
}
