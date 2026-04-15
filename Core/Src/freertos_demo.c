#include "freertos_demo.h"
/*freeRTOS相关头文件*/ 
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
/*  需要用到的其他头文件 */
#include "main.h"
#include "Get_and_Convert.h"
#include "iic_soft.h"
#include "Safety_Protection.h"





extern UART_HandleTypeDef huart1;


/* 启动任务的配置 */
#define START_TASK_STACK_SIZE 128
#define START_TASK_PRIORITY 1
TaskHandle_t StartTaskHandle;
StackType_t StartTaskStack[START_TASK_STACK_SIZE];// 自己创建静态任务堆栈，以数组形式存储
static StaticTask_t StartTaskTCB;                 // 自己创建静态任务的TCB结构体类型
void start_task(void *pvParameters);


/* 任务1的配置 */
#define TASK1_STACK_SIZE 256
#define TASK1_PRIORITY 2
TaskHandle_t Task1Handle;
StackType_t Task1Stack[TASK1_STACK_SIZE];
static StaticTask_t Task1TCB;                 
void task1(void *pvParameters);


/* 任务2的配置 */
#define TASK2_STACK_SIZE 512
#define TASK2_PRIORITY 3
TaskHandle_t Task2Handle;
StackType_t Task2Stack[TASK2_STACK_SIZE];
static StaticTask_t Task2TCB;                
void task2(void *pvParameters);


/* 任务3的配置 */
#define TASK3_STACK_SIZE 512
#define TASK3_PRIORITY 4
TaskHandle_t Task3Handle;
StackType_t Task3Stack[TASK3_STACK_SIZE];
static StaticTask_t Task3TCB;                 
void task3(void *pvParameters);

/* ===========静态创建方式，需要手动指定2个资源：空闲任务和定时器任务 */
/* 空闲任务的配置*/
StackType_t IdleTaskStack[configMINIMAL_STACK_SIZE];
static StaticTask_t IdleTaskTCB;          

/* 定时器任务的配置 */
StackType_t TimerTaskStack[configTIMER_TASK_STACK_DEPTH];
static StaticTask_t TimerTaskTCB;


/* 分配空闲任务的资源 */
void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                        StackType_t ** ppxIdleTaskStackBuffer,
                                        uint32_t * pulIdleTaskStackSize )

                                        {
                                          *ppxIdleTaskTCBBuffer = &IdleTaskTCB; // 提供空闲任务TCB的内存地址 
                                          * ppxIdleTaskStackBuffer = IdleTaskStack; // 提供空闲任务堆栈的内存地址
                                          * pulIdleTaskStackSize = configMINIMAL_STACK_SIZE; // 提供空闲任务堆栈大小
                                        }
/* 分配定时器任务的资源 */
void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                         StackType_t ** ppxTimerTaskStackBuffer,
                                         uint32_t * pulTimerTaskStackSize )

                                         {
                                           *ppxTimerTaskTCBBuffer = &TimerTaskTCB; // 提供定时器任务TCB的内存地址 
                                           * ppxTimerTaskStackBuffer = TimerTaskStack; // 提供定时器任务堆栈的内存地址
                                           * pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH; // 提供定时器任务堆栈大小
                                         }











/*  
    @description: 启动FreeRTOS
    @return: 无
*/
void freertos_start(void)
{
    /* 1.创建一个启动任务*/
                    
   StartTaskHandle = xTaskCreateStatic( (TaskFunction_t) start_task,            //任务函数地址
                     (char * ) "StartTask",                                     //任务名称
                     (uint32_t) START_TASK_STACK_SIZE,                          //任务堆栈大小，单位4个字节
                     (void *)  NULL,                                            //传递给任务函数的参数
                     (UBaseType_t) START_TASK_PRIORITY,                         //任务优先级
                     (StackType_t * ) StartTaskStack,                           //任务堆栈地址
                     (StaticTask_t * ) &StartTaskTCB );                         //自己创建静态任务，提供堆栈和TCB的内存地址
    /* 2.启动任务调度器：会自动创建空闲任务和软件定时器（如果开启了软件定时器），静态创建的方式需要去
    实现两个分配资源的接口函数*/
    vTaskStartScheduler();
    
}


/*  
    @description: 启动任务，用来创建其他三个任务
    @return: 无
*/
void start_task(void *pvParameters)
{

 /*进入临界区 ；临界区的代码不会被打断，确保被完整执行，防止在创建任务时，其他任务正在运行*/
 taskENTER_CRITICAL();  
 /* 创建3个任务 */
 Task1Handle = xTaskCreateStatic( (TaskFunction_t) task1,            
                     (char * ) "Task1",                                    
                     (uint32_t) TASK1_STACK_SIZE,                          
                     (void *)  NULL,                                            
                     (UBaseType_t) TASK1_PRIORITY,                         
                     (StackType_t * ) Task1Stack,                          
                     (StaticTask_t * ) &Task1TCB );                         
 Task2Handle = xTaskCreateStatic( (TaskFunction_t) task2,            
                     (char * ) "Task2",                                    
                     (uint32_t) TASK2_STACK_SIZE,                          
                     (void *)  NULL,                                            
                     (UBaseType_t) TASK2_PRIORITY,                         
                     (StackType_t * ) Task2Stack,                          
                     (StaticTask_t * ) &Task2TCB );                             
 Task3Handle = xTaskCreateStatic( (TaskFunction_t) task3,            
                     (char * ) "Task3",                                    
                     (uint32_t) TASK3_STACK_SIZE,                          
                     (void *)  NULL,                                            
                     (UBaseType_t) TASK3_PRIORITY,                         
                     (StackType_t * ) Task3Stack,                          
                     (StaticTask_t * ) &Task3TCB );  
                     
    

   


    /*推出临界区*/
    taskEXIT_CRITICAL();  


     /* 启动任务只需要执行一次即可，创建完成后删除自己 */
    vTaskDelete(StartTaskHandle);
}



/*  
    @description: 任务1；
    @return: 无
*/
void task1(void *pvParameters)
{
    while(1)
    {
        
        
    }
}




/*  
    @description: 任务2；打印出当前电池组的 SOC 值
    @return: 无
*/
void task2(void *pvParameters)
{
    while(1)
    {
        // 1. 运行你写的这个均衡逻辑函数
        Bms_Turn_On_Balance_Mode();
        // 2. 打印信息
        // 技巧：我们可以直接读取硬件寄存器，看看现在到底谁在均衡
        uint8_t real_bal_status = 0;
        // 读取硬件寄存器，判断是否有故障
        uart_printf("---Fault Status---\r\n");
        BQ76920_Diagnose_Fault();
        BQ76920_Read_Reg(0x01, &real_bal_status); 

        uart_printf("\r\n--- BMS Monitor ---\r\n");
        uart_printf("V: %dmV, %dmV, %dmV, %dmV, %dmV\r\n", 
                    BQ76920_Data.Cell_V[0], BQ76920_Data.Cell_V[1], 
                    BQ76920_Data.Cell_V[2], BQ76920_Data.Cell_V[3], 
                    BQ76920_Data.Cell_V[4]);
        uart_printf("SOC: %d%% | Current: %d mA\r\n",(uint16_t)BQ76920_Data.SOC, BQ76920_Data.CC);
        uart_printf("Balance Active Bits: 0x%02X\r\n", real_bal_status);
        Watchdog_Monitor_Data.Task2_RunFlag = 1; // 标记任务2存活
        uint8_t rx_data;
if (HAL_UART_Receive(&huart1, &rx_data, 1, 0) == HAL_OK) {
    if (rx_data == 'U') {
        Bms_Reset_Safety_Lock();
        uart_printf("手动解锁命令已执行\n");
    }
}
        
      
        
        vTaskDelay(2000); // 均衡和打印，2秒一次完美
    }
}



/*  
    @description: 任务3；
    @return: 无
*/
void task3(void *pvParameters)
{
    /*开启ADC*/
    BQ76920_ADC_Init();
    BQ76920_CC_Init();
    /*初始化硬件保护*/
    BQ76920_Hardware_Protection_Init();
    BQ76920_Get_Offset_Gain(); 
    uint8_t sys_stat = 0;
        BQ76920_Read_Reg(0x05, &sys_stat); // 读取系统状态寄存器
        uart_printf("SYS_STAt: 0x%02X\r\n", sys_stat); // 打印状态寄存器，方便调试
        
    while(1)
    {
       
       BQ76920_Get_Voltage(BQ76920_Data.Cell_V);
       //读取电压并且更新最大电压和最小电压
      Check_Safety_protection();
        // 注意：这个函数的执行频率必须和它内部计算公式的时间常数对齐
        BQ76920_Get_SOC();
        BQ76920_GET_SOH();
        Watchdog_Monitor_Data.Task3_RunFlag = 1; // 任务3打卡
        // 只有当两个任务都打过卡时，才喂狗
        if (Watchdog_Monitor_Data.Task2_RunFlag && Watchdog_Monitor_Data.Task3_RunFlag) {
            HAL_IWDG_Refresh(&hiwdg);
            uart_printf("喂狗成功\r\n");
            // 喂完后重置，要求两个任务在下一轮重新打卡
            Watchdog_Monitor_Data.Task2_RunFlag = 0;
            Watchdog_Monitor_Data.Task3_RunFlag = 0;
        }
        uint8_t sys_stat = 0;
        BQ76920_Read_Reg(0x05, &sys_stat); // 读取系统状态寄存器
        uart_printf("SYS_STAt: 0x%02X\r\n", sys_stat); // 打印状态寄存器，方便调试

       uint8_t hi, lo;
       BQ76920_Read_Reg(0x32, &hi);
        BQ76920_Read_Reg(0x33, &lo);
         uart_printf("CC_HI=0x%02X, CC_LO=0x%02X\n", hi, lo);
         uint8_t dd;
        BQ76920_Read_Reg(0x0B, &dd);
        uart_printf("CC_CFG=0x%02X\n", dd);
        // can 发送电压
        BMS_CAN_SendData();
        vTaskDelay(1000);
    }
    
}







