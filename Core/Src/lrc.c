#include "lrc.h"


uint8_t LRC_Parse_Line(const char *lrc_line, LRC_Line_t *lrc_data)
{
    if (lrc_line == NULL || lrc_data == NULL) return 1;

    // 清空解析结构体
    memset(lrc_data, 0, sizeof(LRC_Line_t));

    uint8_t min, sec, ms;
    char lyric_temp[LRC_LYRIC_BUF];
    char *lyric_start = NULL;

    // 第一步：解析时间戳 [mm:ss.ms]
    if (sscanf(lrc_line, "[%d:%d.%d]%s", &min, &sec, &ms,lrc_data->lyric) != 4)
    {
        return 1; // 时间戳格式错误
    }
    lrc_data->time_ms = min * 60 * 1000 + sec * 1000 + ms * 10; // 统一转为毫秒

    // 第二步：提取歌词内容（修复原%S被空格截断的问题）
    /*
    lyric_start = strchr(lrc_line, ']');
    if (lyric_start == NULL) return 1;
    lyric_start++; // 跳过']'符号
    */
    
    /*
    // 去除歌词开头的空格，拷贝有效内容
    while (*lyric_start == ' ' || *lyric_start == '\t') lyric_start++;
    strncpy(lrc_data->lyric, lyric_start, LRC_LYRIC_BUF - 1);
    */
    

    return 0;
}

/**
 * @brief  读取并解析LRC歌词文件的核心函数
 * @param  lrc_file_path: LRC文件路径（如"0:程艾影-赵雷.lrc"）
 * @retval FRESULT: FatFs错误码，FR_OK为成功
 * @note   1. 内部自动处理挂载（重复挂载不影响）、文件打开/关闭
 *         2. 大结构体使用static修饰，避免栈溢出
 *         3. 解析结果通过串口打印，可扩展为回调函数输出
 */
FRESULT LRC_Read_Parse(const TCHAR *lrc_file_path)
{
    // 静态修饰大结构体，避免栈溢出（存储在全局数据区）
    static FATFS myFatFs;
    static FIL myFile;
    FRESULT f_res = FR_DISK_ERR;
    TCHAR readBuf[LRC_BUF_SIZE]; // 静态缓冲区，减少栈占用
    LRC_Line_t lrc_data;

    // 1. 参数校验：路径不能为空
    if (lrc_file_path == NULL || *lrc_file_path == '\0')
    {
        printf("LRC Error: File path is NULL!\r\n");
        return FR_INVALID_PARAMETER;
    }

    // 2. 挂载文件系统（重复挂载会返回FR_OK，不影响现有挂载）
    f_res = f_mount(&myFatFs, "0:", 1);
    if (f_res != FR_OK)
    {
        printf("LRC Mount Fail! Error code: %d\r\n", f_res);
        return f_res;
    }
    printf("LRC Mount Success!\r\n");

    // 3. 打开LRC文件（只读模式）
    f_res = f_open(&myFile, lrc_file_path, FA_OPEN_EXISTING | FA_READ);
    if (f_res != FR_OK)
    {
        printf("LRC Open File Fail! Path: %s, Error code: %d\r\n", lrc_file_path, f_res);
        f_mount(NULL, "0:", 1); // 挂载失败时解除挂载，释放资源
        return f_res;
    }
    printf("LRC Open File Success! Path: %s\r\n", lrc_file_path);

    // 4. 逐行读取并解析LRC文件
    printf("================ LRC Content ================\r\n");
    while (f_gets(readBuf, sizeof(readBuf), &myFile) != NULL)
    {
        // 解析单行LRC
        if (LRC_Parse_Line(readBuf, &lrc_data) == 0)
        {
            // 打印解析结果（可替换为回调函数，实现歌词数据传递）
            printf("Time: %d ms | Lyric: %s\r\n", lrc_data.time_ms, lrc_data.lyric);
        }
        else
        {
            // 非标准LRC行（如注释、空行），跳过并打印原始内容
            printf("LRC Invalid Line: %s", readBuf);
        }
    }
    printf("=============================================\r\n");

    // 5. 资源释放：关闭文件+解除挂载（按需选择是否解除挂载）
    f_close(&myFile);
    f_mount(NULL, "0:", 1); // 解除挂载，若其他模块需使用SD卡可注释此行
    printf("LRC File Closed & Unmount Success!\r\n");

    return FR_OK;
}
