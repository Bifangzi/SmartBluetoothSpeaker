#ifndef __LRC_H__
#define __LRC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"
#include "string.h"
#include "fatfs.h"

// 缓冲区大小定义
#define LRC_BUF_SIZE    128
#define LRC_LYRIC_BUF   128

/**
 * @brief  LRC歌词行解析结构体
 * @note   存储解析后的时间戳和歌词内容
 */
typedef struct
{
    uint32_t time_ms;               // 时间戳，单位：毫秒
    char lyric[LRC_LYRIC_BUF];      // 歌词内容 
} LRC_Line_t;

uint8_t LRC_Parse_Line(const char *lrc_line, LRC_Line_t *lrc_data);
FRESULT LRC_Read_Parse(const TCHAR *lrc_file_path);

#ifdef __cplusplus
}
#endif
#endif
