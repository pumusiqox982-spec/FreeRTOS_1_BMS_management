#include "bootloader.h"








/******************************ʹ������ע������************************ */
/**
 * 1.�κ�flash���м������ܣ���д��flashǰ��Ҫ�Ƚ�����д����ɺ���Ҫ���¼���
 * 2.д��flashǰ��Ҫ�Ȳ�������������д�룬���԰���ҳ����Ҳ����ȫ������
 * 
 */



/* ȫ�ֱ���*/
extern UART_HandleTypeDef huart1;
uint8_t uart_rec_buff[BOOTLOADER_UART_RECEIVE_BUFFER_SIZE];  /* UART���ջ����� */
uint16_t uart_rec_size = 0;  /* UART�������ݴ�С */
uint32_t total_received_count = 0;/* ȫ�ֱ����������ں������� static�� */

// ��¼��ǰд������ƫ���� (дһ��Ųһ�£�Ų���ĸ�λ�þ���ƫ����)
uint32_t current_write_address = 0;

// ĩβ���ܳ��ֵ����ֽ�
uint8_t last_byte_flag = 0; // ��־λ����¼�Ƿ��������ĵ��ֽ�
uint8_t last_byte = 0; // ��¼�ϴν��յ����һ���ֽڣ�����ǵ����ֽڣ��ͱ���������ȴ���һ�ν���ʱ����һ���ֽ�ƴ�ӳ�16λ����һ��д��flash

// ��¼��ǰһ�ν������ݵ�ʱ��
uint32_t last_rec_time = 0;


// �ϴ�����һ���ֽڣ���Ҫ������ֽ���Ϊ��һλд��
static void Int_flash_write_with_last_byte(void)
{
        // �ϴ�����һ���ֽڣ���Ҫ������ֽ���Ϊ��һλд��
        for (uint16_t i = 0; i < uart_rec_size; i+=2)
        {
          uint16_t data = 0;
          if (i == 0)
          {
            // ��һ���ֽڣ���Ҫ���ϴ��������ֽ�ƴ�ӳ�16λ����
            data = last_byte | (uart_rec_buff[i] << 8);
          }
          else
          {
            // �����ֽڣ�ֱ��д��
            data = uart_rec_buff[i-1] | (uart_rec_buff[i] << 8);
          }
          // д������
           HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, APP_FLASH_START_ADDRESS + i + current_write_address, data);
        }
}


// û�������ֽڣ�����д��
static void Int_flash_write_with_no_last_byte(void)
{
  // û�������ֽڣ�����д��
 for (uint16_t i = 0; i < uart_rec_size; i+=2)
  {
     uint16_t data = 0;
    // �����ɶ�,��Ϊ�����������ֽڣ�i+1�ᳬ����Χ������ֻ�ܶ�ż����ʣ������Ҫ���last_byte_flag���ȴ���һ�ν���ʱ����һ���ֽ�ƴ�ӳ�16λ����һ��д��flash
    if(i + 1 < uart_rec_size)
    {
      data = uart_rec_buff[i] | (uart_rec_buff[i+1] << 8);
       // д������
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, APP_FLASH_START_ADDRESS + i + current_write_address, data);
    }
   
  }
}


// ������ǰҳ���ж��Ƿ���Ҫ������ǰҳ�������ǰҳ�����µ�һҳ��˵��֮ǰ�Ѿ�д��������ˣ���Ҫ�Ȳ���
static void Int_flash_erase(void)
{
    uint8_t is_erase = 0;
    uint32_t page_address = 0;

    // ��鱾�ν��հ����ǵ��ֽ��Ƿ��Ϊ0xFF
    for (uint16_t i = 0; i < uart_rec_size; i++)
    {
        uint8_t data = *(volatile uint8_t *)(APP_FLASH_START_ADDRESS + i + current_write_address);
        if (data != 0xFF)
        {
            is_erase = 1;
            page_address = ((APP_FLASH_START_ADDRESS + current_write_address) / 1024) * 1024;
            break;
        }
    }

    if (is_erase == 1)
    {
        FLASH_EraseInitTypeDef erase_init;
        erase_init.TypeErase   = FLASH_TYPEERASE_PAGES;
        erase_init.Banks       = FLASH_BANK_1;           // STM32F103ֻ��Bank1
        erase_init.PageAddress = page_address;
        erase_init.NbPages     = 1;

        uint32_t page_error = 0;
        // ͬ�������������ڲ���ȴ�BSY�����������HAL״̬
        if (HAL_FLASHEx_Erase(&erase_init, &page_error) != HAL_OK)
        {
            // ����ʧ�ܴ��������Խ�����ѭ�����¼���󣨽�������ֹͣ����д�룩
            Error_Handler();   // �����Զ��������
        }
        // ע�⣺������Ҫ while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY));
    }
}

// �ж�д����ż���������������Ƿ��������ֽڣ�ƴ�Ӻ�д��
static void Int_flash_write_half_word(void)
{
   if((uart_rec_size + last_byte_flag )% 2 == 0)
     {
       if(last_byte_flag == 1)
       {
        Int_flash_write_with_last_byte();
        // ����ƫ�Ƶ�ַ��Ϊ��һ����׼��
        current_write_address += uart_rec_size + 1;
        last_byte_flag = 0; // �ϴ�����һ���ֽڣ���λ�����һ��0
       }
       else
       {
          // û�������ֽڣ�����д��
          Int_flash_write_with_no_last_byte();
          // ����ƫ�Ƶ�ַ��Ϊ��һ����׼��
           current_write_address += uart_rec_size;
           last_byte_flag = 0; // �ϴ�����һ���ֽڣ���λ�����һ��0
       }
     }
     else
     {
            if(last_byte_flag == 1)
          {
             Int_flash_write_with_last_byte();

            // �޸�ʣ�µ��ֽ�
             last_byte = uart_rec_buff[uart_rec_size - 1]; // ���һ���ֽ�����
             last_byte_flag = 1; // ��λ�����һ��1
             current_write_address += uart_rec_size + 1;
          }
        else
        {
          Int_flash_write_with_no_last_byte();
          last_byte = uart_rec_buff[uart_rec_size - 1]; // ���һ���ֽ�����
          last_byte_flag = 1; // ��λ�����һ��1
           current_write_address += uart_rec_size - 1;
        }
     }
}



/**
 * @brief  ���ڽ����жϻص����������յ������ַ�ʱ���ᴥ���жϵĺ���
 * ������1. huart�����ھ��ָ�� 2. Size�����յ������ݴ�С
 * ����ֵ����
 */


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
   if(huart->Instance == USART1)
   {
      // ���յ����ݣ���¼��ǰʱ�� ��λ��ms
      last_rec_time = HAL_GetTick();

      // ����ʵ�ʽ��ܵ����ݴ�С
     uart_rec_size = Size;
     total_received_count += uart_rec_size;
     
      
     // �����յ�������д��flash
     // 1.����flash
     HAL_FLASH_Unlock();

     // 2.�жϵ�ǰд��ĵ�ַ�Ƿ�Ϊ�µ�һҳ����������µ�һҳ�����Ȳ�����ǰҳ
     // ��ǰҳ�������FFF��˵�����µ�һҳ����������µ�һҳ��˵��֮ǰ�Ѿ�д��������ˣ���Ҫ�Ȳ���
     //Int_flash_erase();
      
     // 2.2 ʹ��16λд�룬ÿ�ν��հ���ֽڣ�Ҫƴ�ӣ����ǰ��ֶ��룬������2���ֽ������ֽ�д�루16λ�������ҵ�ַ������ż����2�ı�����ַ��

     // 2.2.1 �жϵ�ǰд��������Ƿ�ż��
     Int_flash_write_half_word();


     // 3.���¼���flash
     HAL_FLASH_Lock();
     
     __HAL_UART_CLEAR_OREFLAG(&huart1);//��մ���ʹ��֮ǰ������,����ǹ��ش������
     __HAL_UART_CLEAR_IDLEFLAG(&huart1);// ������б�־
      // ��ս��ջ�����
      memset(&uart_rec_buff, 0, BOOTLOADER_UART_RECEIVE_BUFFER_SIZE);
      HAL_UARTEx_ReceiveToIdle_IT(&huart1, uart_rec_buff, BOOTLOADER_UART_RECEIVE_BUFFER_SIZE);
   }
}




/**
 * @brief 擦除应用区（A区）。
 *        由于 B 区已调整为 25KB，A 区起始地址为 `APP_FLASH_START_ADDRESS`，
 *        最大大小为 `APP_MAX_SIZE`（39KB）。
 * @retval HAL_OK 成功 / HAL_ERROR 失败
 */
static HAL_StatusTypeDef Int_flash_erase_app_area(void)
{
  uint32_t first_page = APP_FLASH_START_ADDRESS / 1024;
  uint32_t last_page  = (APP_FLASH_START_ADDRESS + APP_MAX_SIZE - 1) / 1024;
    uint32_t page_error;

    HAL_FLASH_Unlock();

    for (uint32_t page = first_page; page <= last_page; page++)
    {
        FLASH_EraseInitTypeDef erase;
        erase.TypeErase   = FLASH_TYPEERASE_PAGES;
        erase.Banks       = FLASH_BANK_1;
        erase.PageAddress = page * 1024;
        erase.NbPages     = 1;

        if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK)
        {
            uart_printf("����ҳ 0x%08lX ʧ��\r\n", page * 1024);
            HAL_FLASH_Lock();
            return HAL_ERROR;
        }
    }

    HAL_FLASH_Lock();
    uart_printf("App 区擦除完成（最大 %lu bytes）\r\n", (unsigned long)APP_MAX_SIZE);
    return HAL_OK;
}





/**
 * @brief  ���ڽ��� => ׼������A����
 * ����Э���ȶ��Բ� ���ͳ��ļ����׶�ʧ
 * DMA��һ�㣬���ǻ����޷������ʧ����
 * �޸Ĳ����ʿ��Խ����ʧ����
 * ��þͲ�Ҫ���ж������ӡ
 */

 void Int_bootloader_receive_app(void)
 {
      
     __HAL_UART_CLEAR_OREFLAG(&huart1);//��մ���ʹ��֮ǰ������,����ǹ��ش������
     __HAL_UART_CLEAR_IDLEFLAG(&huart1);// ������б�־


    current_write_address = 0;      // ����ƫ�Ƶ�ַ
    last_byte_flag = 0;              // �����ֽڱ�־
    total_received_count = 0;        // ���ü���

    // �������� App ���򣨿�ѡ��������Ҫ�����Ƿ�ÿ�ζ�����
   // Int_flash_erase_app_area();
    last_rec_time = HAL_GetTick(); 
    

     // �����жϵĽ��մ��ں���������������Խ��ղ����������ݣ������յ������ַ�ʱ���ᴥ���ж�
     HAL_UARTEx_ReceiveToIdle_IT(&huart1, uart_rec_buff, BOOTLOADER_UART_RECEIVE_BUFFER_SIZE);
 }




 /**
 * @brief  ��ת��Ӧ�ó���ִ��
 * ��������
 * ����ֵ��uint8_t���ͣ�0��ʾ��ת�ɹ���1��ʾ��תʧ��
 * ��Ϊ�ҵ�оƬ��c8t6�������ҵ�ջ��ʼ��ַһ����0x20000000����С��20k��0x5000��������ջ����ַ��0x20005000
 * cpu堆栈初始化地址（MSP）在 APP_FLASH_START_ADDRESS，复位向量在 APP_FLASH_START_ADDRESS + 4（例如 0x08006404）
 */
uint8_t Int_bootloader_jump_to_app(void)  
{
  typedef void (*pFunc)(void);
  // 1.��ȡӦ�ó����ջ����ַ��Ҳ����Ӧ�ó������ڵ�ַ
  uint32_t app_stack_top_address = *(volatile uint32_t *)(APP_FLASH_START_ADDRESS);
  // 2.��ȡӦ�ó�������ô���������ַ(�жϸ�λ��ַ)
  uint32_t app_reset_handler_address = *(volatile uint32_t *)(APP_FLASH_START_ADDRESS + 4);
  // 3.У��ջ����ַ (������RAM��Χ��)
  if(app_stack_top_address < RAM_BASE_ADDRESS || app_stack_top_address > RAM_TOP_ADDRESS)
  {
    uart_printf("Ӧ�ó����ջ����ַ���Ϸ�����תʧ�ܣ�\r\n");
    return 1;
  }
  // 4.У�鸴λ�жϵ�ַ
  if(app_reset_handler_address < APP_FLASH_START_ADDRESS || app_reset_handler_address > (APP_FLASH_START_ADDRESS + APP_MAX_SIZE))
  {
    uart_printf("Ӧ�ó���ĸ�λ�жϵ�ַ���Ϸ�����תʧ�ܣ�\r\n");
    return 1;
  }

  // 5.ע��bootloader���򣬹��жϣ���ʵ���Բ�������Ϊ��ת֮�󣬺��������³�ʼ����
  __disable_irq();
  // 6.ע��hal��
  HAL_DeInit();
  // 7.��������ջָ��MSPΪӦ�ó����ջ����ַ
  __set_MSP(app_stack_top_address);
  // 8.�����ж�������ָ��ΪӦ�ó�����ж���������ַ
  SCB->VTOR = APP_FLASH_START_ADDRESS;

  // 9.��ת��Ӧ�ó���ĸ�λ�жϵ�ַ
  pFunc jump_to_app = (pFunc)app_reset_handler_address;
  jump_to_app();
  return 0;
}


/**
 * @brief �����flashҳ
 * @param page_address ��ʼҳ��ַ
 * @param pages Ҫ������ҳ��
 */
void Int_bootloader_erase_app(uint32_t page_address, uint32_t pages)
{
  //  ����flash
  HAL_FLASH_Unlock();

        FLASH_EraseInitTypeDef erase_init;
        erase_init.TypeErase   = FLASH_TYPEERASE_PAGES;
        erase_init.Banks       = FLASH_BANK_1;           // STM32F103ֻ��Bank1
        erase_init.PageAddress = page_address;
        erase_init.NbPages     = pages;

        uint32_t page_error = 0;
        // ͬ�������������ڲ���ȴ�BSY�����������HAL״̬
        if (HAL_FLASHEx_Erase(&erase_init, &page_error) != HAL_OK)
        {
            // ����ʧ�ܴ��������Խ�����ѭ�����¼���󣨽�������ֹͣ����д�룩
            Error_Handler();   // �����Զ��������
        }

 //  ����flash
 HAL_FLASH_Lock();
}


