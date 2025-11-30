/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "fatfs.h"
#include "sdio.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
#include "lrc.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// SD卡的FatFS文件系统挂载、格式化、读写测试
void FatFsTest(void)
{
    static FATFS myFatFs;                                                 // FatFs 文件系统对象; 这个结构体占用598字节，有点大，需用static修饰(存放在全局数据区), 避免stack溢出
    static FIL myFile;                                                    // 文件对象; 这个结构体占用570字节，有点大，需用static修饰(存放在全局数据区), 避免stack溢出
    static FRESULT f_res;                                                 // 文件操作结果
    static uint32_t num;                                                  // 文件实际成功读写的字节数
    static uint8_t aReadData[1024] = {0};                                 // 读取缓冲区; 这个数组占用1024字节，需用static修饰(存放在全局数据区), 避免stack溢出
    static uint8_t aWriteBuf[] =  "Test, this is FatFs Test ! \r\n";      // 要写入的数据
 
    // 重要的延时：避免烧录期间的复位导致文件读写、格式化等错误
    HAL_Delay(1000);                                                      // 重要：稍作延时再开始读写测试; 避免有些仿真器烧录期间的多次复位，短暂运行了程序，导致下列读写数据不完整。
 
    // 1、挂载测试：在SD卡挂载文件系统
    printf("\r\n\r\n");
    printf("1、Load FatFs test ****** \r\n");
    f_res = f_mount(&myFatFs, "0:", 1);                                   // 在SD卡上挂载文件系统; 参数：文件系统对象、驱动器路径、读写模式(0只读、1读写)
    if (f_res == FR_NO_FILESYSTEM)                                        // 检查是否已有文件系统，如果没有，就格式化创建创建文件系统
    {
        printf("The SD card has no file system,Strating to format......\r\n");
        static uint8_t aMountBuffer[4096];                                // 格式化时所需的临时缓存; 块大小512的倍数; 值越大格式化越快, 如果内存不够，可改为512或者1024; 当需要在函数内定义这种大缓存时，要用static修饰，令缓存存放在全局数据区内，不然，可能会导致stack溢出。
        f_res = f_mkfs("0:", 0, 0, aMountBuffer, sizeof(aMountBuffer));   // 格式化SD卡; 参数：驱动器、文件系统(0-自动\1-FAT12\2-FAT16\3-FAT32\4-exFat)、簇大小(0为自动选择)、格式化临时缓冲区、缓冲区大小; 格式化前必须先f_mount(x,x,1)挂载，即必须用读写方式挂载; 如果SD卡已格式化，f_mkfs()的第2个参数，不能用0自动，必须指定某个文件系统。
        if (f_res == FR_OK)                                               // 格式化 成功
        {
            printf("SD card format: success \r\n");
            f_res = f_mount(NULL, "0:", 1);                               // 格式化后，先取消挂载
            f_res = f_mount(&myFatFs, "0:", 1);                           // 重新挂载
            if (f_res == FR_OK)
                printf("FatFs load success \r\n");                            // 挂载成功
            else
                return;                                                   // 挂载失败，退出函数
        }
        else
        {
            printf("SD card format: failed \r\n");                              // 格式化 失败
            return;
        }
    }
    else if (f_res != FR_OK)                                              // 挂载异常
    {
        printf("FatFs 挂载异常: %d; Check if MX_SDIO_SD_Init() has beed change to 1B是否已修改1B\r", f_res);
        return;
    }
    else                                                                  // 挂载成功
    {
        if (myFatFs.fs_type == 0x03)                                      // FAT32; 1-FAT12、2-FAT16、3-FAT32、4-exFat
            printf("SD card has file system卡已有文件系统: FAT32\n");
        if (myFatFs.fs_type == 0x04)                                      // exFAT; 1-FAT12、2-FAT16、3-FAT32、4-exFat
            printf("SD卡已有文件系统: exFAT\n");                         
        printf("FatFs 挂载成功 \r\n");                                    // 挂载成功
    }
 
    // 2、写入测试：打开或创建文件，并写入数据
    printf("\r\n");
    printf("2:打开或创建文件,并写入数据 ****** \r\n");
    f_res = f_open(&myFile, "0:text.txt", FA_CREATE_ALWAYS | FA_WRITE);   // 打开文件; 参数：要操作的文件对象、路径和文件名称、打开模式;
    if (f_res == FR_OK)
    {
        printf("打开文件 成功 \r\n");
        printf("写入测试:");
        f_res = f_write(&myFile, aWriteBuf, sizeof(aWriteBuf), &num);     // 向文件内写入数据; 参数：文件对象、数据缓存、申请写入的字节数、实际写入的字节数
        if (f_res == FR_OK)
        {
            printf("写入成功  \r\n");
            printf("已写入字节数: %d \r\n", num);                         // printf 写入的字节数
            printf("已写入的数据: %s \r\n", aWriteBuf);                   // printf 写入的数据; 注意，这里以字符串方式显示，如果数据是非ASCII可显示范围，则无法显示
        }
        else
        {
            printf("写入失败 \r\n");                                      // 写入失败
            printf("Error code: %d\r\n", f_res);                           // printf 错误编号
        }
        f_close(&myFile);                                                 // 不再读写，关闭文件
    }
    else
    {
        printf("Open file fail: %d\r\n", f_res);
    }
 
    // 3、读取测试：打开已有文件，读取其数据
    printf("3、读取测试: 打开刚才的文件，读取其数据 ****** \r\n");
    f_res = f_open(&myFile, "0:text.txt", FA_OPEN_EXISTING | FA_READ);    // 打开文件; 参数：文件对象、路径和名称、操作模式; FA_OPEN_EXISTING：只打开已存在的文件; FA_READ: 以只读的方式打开文件
    if (f_res == FR_OK)
    {
        printf("打开文件 成功 \r\n");
        f_res = f_read(&myFile, aReadData, sizeof(aReadData), &num);      // 从文件中读取数据; 参数：文件对象、数据缓冲区、请求读取的最大字节数、实际读取的字节数
        if (f_res == FR_OK)
        {
            printf("读取数据 成功 \r\n");
            printf("已读取字节数: %d \r\n", num);                         // printf 实际读取的字节数
            printf("读取到的数据: %s\r\n", aReadData);                    // printf 实际数据; 注意，这里以字符串方式显示，如果数据是非ASCII可显示范围，则无法显示
        }
        else
        {
            printf("读取 失败  \r\n");                                    // printf 读取失败
            printf("错误编号：%d \r\n", f_res);                           // printf 错误编号
        }
    }
    else
    {
        printf("打开文件 失败 \r\n");                                     // printf 打开文件 失败
        printf("错误编号: %d\r\n", f_res);                                // printf 错误编号
    }
 
    f_close(&myFile);                                                     // 不再读写，关闭文件
    f_mount(NULL, "0:", 1);                                               // 不再使用文件系统，取消挂载文件系统
}

// 获取SD卡信息
// 注意: 本函数需要在f_mount()执行后再调用，因为CubeMX生成的FatFs代码, 会在f_mount()函数内对SD卡进行初始化
void SDCardInfo(void)
{
    HAL_SD_CardInfoTypeDef pCardInfo = {0};                    // SD卡信息结构体
    uint8_t status = HAL_SD_GetCardState(&hsd);                // SD卡状态标志值
    if (status == HAL_SD_CARD_TRANSFER)
    {
        HAL_SD_GetCardInfo(&hsd, &pCardInfo);                  // 获取 SD 卡的信息
        printf("\r\n");
        printf("*** 获取SD卡信息 *** \r\n");
        printf("卡类型: %d \r\n", pCardInfo.CardType);         // 类型返回：0-SDSC、1-SDHC/SDXC、3-SECURED
        printf("卡版本: %d \r\n", pCardInfo.CardVersion);      // 版本返回：0-CARD_V1、1-CARD_V2
        printf("块数量: %d \r\n", pCardInfo.BlockNbr);         // 可用的块数量
        printf("块大小: %d \r\n", pCardInfo.BlockSize);        // 每个块的大小; 单位：字节
        printf("卡容量: %lluG \r\n", ((uint64_t)pCardInfo.BlockSize * pCardInfo.BlockNbr) / 1024 / 1024 / 1024);  // 计算卡的容量; 单位：GB
    }
}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_SDIO_SD_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
  FatFsTest();      // FatFs文件系统挂载、格式化、读写测试
  SDCardInfo();    // 获取SD卡信息                                              // FatFs 文件系统对象; 这个结构体占用598字节，有点大，需用static修饰(存放在全局数据区), 避免stack溢出

  FRESULT res;
  res = LRC_Read_Parse("0:程艾影-赵雷.lrc"); // 读取并解析LRC歌词文件
  if (res == FR_OK)
    {
        printf("LRC Parse (No Callback) Success!\r\n");
    }
    else
    {
        printf("LRC Parse Fail! Error code: %d\r\n", res);
    }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // 切换PC13引脚状态
    // printf("System running...\r\n");
    HAL_Delay(500); // 延时500毫秒
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM2 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM2) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
