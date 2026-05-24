#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__


/* 需要的头文件*/
#include "main.h"
#include "string.h"

/* 宏变量定义*/
#define BOOTLOADER_UART_RECEIVE_BUFFER_SIZE 256  /* 接收缓冲区长度 */

// 程序写入的起始地址 => A区的起始位置
// B区调整为 25KB (0x6400)，因此 A区起始地址为 0x08006400
#define APP_FLASH_START_ADDRESS 0x08006400 // A区，也就是应用区

// RAM配置 (20K RAM for STM32F103C8T6)
#define RAM_BASE_ADDRESS 0x20000000
#define RAM_SIZE         0x5000      // 20K
#define RAM_TOP_ADDRESS  (RAM_BASE_ADDRESS + RAM_SIZE)  // 0x20005000

// 应用程序的最大大小（整个 Flash 64KB - B区25KB = 39KB）
// 39KB = 0x9C00
#define APP_MAX_SIZE     0x9C00     // 39K

void Int_bootloader_receive_app(void);

uint8_t Int_bootloader_jump_to_app(void);

void Int_bootloader_erase_app(uint32_t page_address, uint32_t pages);

#endif