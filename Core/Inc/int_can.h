#ifndef __INT_CAN_H__
#define __INT_CAN_H__


#include "main.h"
#include <string.h>


typedef struct
{
    CAN_RxHeaderTypeDef RxHeader;       // 消息ID
    uint8_t data[8];   // 数据内容，CAN帧最大8字节
} CAN_REC_DATA;



/**
 * @brief can通信初始化
 * 需要配置过滤器 can要手动开启（因为默认是休眠状态）
 */
void CAN_Init(void);



/**
 * @brief can发送数据
 * @param id 发送的ID
 * @param data 要发送的数据 
 * @param len 数据长度 (最长8字节)
 */
void CAN_SendData(uint16_t id, uint8_t *data, uint8_t len);



/**
 * @brief can接收数据
 * @param rec_data 接收数据的结构体指针 ，数组,最多一次获取3条信息
 * @param data_count 用于 返回 CAN 接收 FIFO 中当前待读取的消息 条数
 */
void CAN_ReceiveData(CAN_REC_DATA *rec_data,uint8_t *data_count);

#endif 
