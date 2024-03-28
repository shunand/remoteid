/**
 * @file sys_log.h
 * @author LokLiang (lokliang@163.com)
 * @brief
 * @version 0.1
 * @date 2023-05-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __SYS_LOG_H__
#define __SYS_LOG_H__

#include "sys_types.h"
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef SYS_LOG_DOMAIN
#define SYS_LOG_DOMAIN "general"
#endif

#ifndef CONFIG_SYS_LOG_LEVEL
#define CONFIG_SYS_LOG_LEVEL SYS_LOG_LEVEL_INF /* 配置日志打印的等级 */
#endif

#define SYS_LOG_LEVEL_OFF 0
#define SYS_LOG_LEVEL_ERR 1
#define SYS_LOG_LEVEL_WRN 2
#define SYS_LOG_LEVEL_INF 3
#define SYS_LOG_LEVEL_DBG 4

#ifndef CONFIG_SYS_LOG_COLOR_ON
#define CONFIG_SYS_LOG_COLOR_ON 1 /* 使能打印颜色 */
#endif

#ifndef CONFIG_SYS_LOG_LOCAL_ON
#define CONFIG_SYS_LOG_LOCAL_ON 1 /* 强制使能打印所在文件位置 */
#endif

#define SYS_PRINT printf
#define SYS_VPRINT vprintf
#define SYS_SPRINT sprintf
#define SYS_SNPRINT snprintf
#define SYS_VSNPRINT vsnprintf

#ifndef CONFIG_SYS_LOG_CONS_ON
#define CONFIG_SYS_LOG_CONS_ON (CONFIG_SYS_LOG_LEVEL > SYS_LOG_LEVEL_OFF) /* 打印控制总开关 */
#endif

#ifndef CONFIG_SYS_LOG_DUMP_ON
#define CONFIG_SYS_LOG_DUMP_ON CONFIG_SYS_LOG_DBG_ON /* 使能打印二进制和十六进制类型 */
#endif

#ifndef CONFIG_SYS_LOG_DBG_ON
#define CONFIG_SYS_LOG_DBG_ON (CONFIG_SYS_LOG_LEVEL >= SYS_LOG_LEVEL_DBG) /* 使能打印调试类型*/
#endif

#ifndef CONFIG_SYS_LOG_INF_ON
#define CONFIG_SYS_LOG_INF_ON (CONFIG_SYS_LOG_LEVEL >= SYS_LOG_LEVEL_INF) /* 使能打印信息类型 */
#endif

#ifndef CONFIG_SYS_LOG_WRN_ON
#define CONFIG_SYS_LOG_WRN_ON (CONFIG_SYS_LOG_LEVEL >= SYS_LOG_LEVEL_WRN) /* 使能打印警告类型 */
#endif

#ifndef CONFIG_SYS_LOG_ERR_ON
#define CONFIG_SYS_LOG_ERR_ON (CONFIG_SYS_LOG_LEVEL >= SYS_LOG_LEVEL_ERR) /* 使能打印错误类型 */
#endif

#ifndef CONFIG_SYS_LOG_ASS_ON
#define CONFIG_SYS_LOG_ASS_ON (CONFIG_SYS_LOG_LEVEL >= SYS_LOG_LEVEL_INF) /* 使能有效性检测 */
#endif

#define CONS_PRINT(FMT, ...)               \
    do                                     \
    {                                      \
        if (CONFIG_SYS_LOG_CONS_ON)        \
        {                                  \
            SYS_PRINT(FMT, ##__VA_ARGS__); \
        }                                  \
    } while (0)

#ifndef CONS_ABORT
#define CONS_ABORT()           \
    do                         \
    {                          \
        volatile int FLAG = 0; \
        do                     \
        {                      \
        } while (FLAG == 0);   \
    } while (0)
#endif

#if (CONFIG_SYS_LOG_COLOR_ON == 1)
#define _COLOR_R "\033[31m"     /* 红 RED    */
#define _COLOR_G "\033[32m"     /* 绿 GREEN  */
#define _COLOR_Y "\033[33m"     /* 黄 YELLOW */
#define _COLOR_B "\033[34m"     /* 蓝 BLUE   */
#define _COLOR_P "\033[35m"     /* 紫 PURPLE */
#define _COLOR_C "\033[36m"     /* 青 CYAN   */
#define _COLOR_RY "\033[41;33m" /* 红底黄字  */
#define _COLOR_END "\033[0m"    /* 结束      */
#else
#define _COLOR_R ""   /* 红 */
#define _COLOR_G ""   /* 绿 */
#define _COLOR_Y ""   /* 黄 */
#define _COLOR_B ""   /* 蓝 */
#define _COLOR_P ""   /* 紫 */
#define _COLOR_C ""   /* 青 */
#define _COLOR_RY ""  /* 红底黄字 */
#define _COLOR_END "" /* 结束 */
#endif

#define _DO_SYS_LOG(FLAG, FMT, ARG...) \
    do                                 \
    {                                  \
        if (FLAG)                      \
        {                              \
            CONS_PRINT(FMT, ##ARG);    \
        }                              \
    } while (0)

#define _FILENAME(FILE) (strrchr(FILE, '/') ? (strrchr(FILE, '/') + 1) : (strrchr(FILE, '\\') ? (strrchr(FILE, '\\') + 1) : FILE))

#define _GEN_DOMAIN "[" SYS_LOG_DOMAIN "] "

#define _SYS_LOG(FLAG, INFO, FMT, ...) _DO_SYS_LOG(FLAG, INFO "%s:%u \t%s =>\t" FMT "\r\n", \
                                                   _FILENAME(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#if (CONFIG_SYS_LOG_LOCAL_ON == 1 || CONFIG_SYS_LOG_DBG_ON == 1)
#define _SYS_LOG_COLOR(FLAG, INFO, COLOR, FMT, ...) _DO_SYS_LOG(FLAG, INFO "%s:%u \t%s =>\t" COLOR FMT _COLOR_END "\r\n", \
                                                                _FILENAME(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#define _SYS_LOG_COLOR(FLAG, INFO, COLOR, FMT, ...) _DO_SYS_LOG(FLAG, INFO "%s =>\t" COLOR FMT _COLOR_END "\r\n", \
                                                                __FUNCTION__, ##__VA_ARGS__)
#endif

#define SYS_LOG_DBG(FMT, ...) _SYS_LOG(CONFIG_SYS_LOG_DBG_ON, "[DBG]   " _GEN_DOMAIN, FMT, ##__VA_ARGS__)
#define SYS_LOG_INF(FMT, ...) _SYS_LOG_COLOR(CONFIG_SYS_LOG_INF_ON, "[INF] | " _GEN_DOMAIN, _COLOR_G, FMT, ##__VA_ARGS__)
#define SYS_LOG_WRN(FMT, ...) _SYS_LOG_COLOR(CONFIG_SYS_LOG_WRN_ON, "[WRN] * " _GEN_DOMAIN, _COLOR_Y, FMT, ##__VA_ARGS__)
#define SYS_LOG_ERR(FMT, ...)                                                        \
    do                                                                               \
    {                                                                                \
        if (CONFIG_SYS_LOG_ERR_ON)                                                   \
        {                                                                            \
            _SYS_LOG_COLOR(1, "[ERR] - " _GEN_DOMAIN, _COLOR_R, FMT, ##__VA_ARGS__); \
            CONS_ABORT();                                                            \
        }                                                                            \
    } while (0)

#define SYS_LOG(FMT, ...)                                                             \
    do                                                                                \
    {                                                                                 \
        if (CONFIG_SYS_LOG_CONS_ON)                                                   \
        {                                                                             \
            CONS_PRINT(_GEN_DOMAIN "- %s:%u, %s =>\t" _COLOR_B FMT _COLOR_END "\r\n", \
                       _FILENAME(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__);   \
        }                                                                             \
    } while (0)

#define SYS_ASSERT_FALSE(EXP, FMT, ...)                                                                       \
    do                                                                                                        \
    {                                                                                                         \
        if (CONFIG_SYS_LOG_ASS_ON && (EXP))                                                                   \
        {                                                                                                     \
            CONS_PRINT("[ASS] # " _GEN_DOMAIN "%s:%u \t%s =>\t" _COLOR_RY "%s" _COLOR_END "; ## " FMT "\r\n", \
                       _FILENAME(__FILE__), __LINE__, __FUNCTION__, #EXP, ##__VA_ARGS__);                     \
            CONS_ABORT();                                                                                     \
        }                                                                                                     \
    } while (0)

#define SYS_ASSERT(EXP, FMT, ...)                                                                                \
    do                                                                                                           \
    {                                                                                                            \
        if (CONFIG_SYS_LOG_ASS_ON && !(EXP))                                                                     \
        {                                                                                                        \
            CONS_PRINT("[ASS] # " _GEN_DOMAIN "%s:%u \t%s =>\t" _COLOR_RY "!(%s)" _COLOR_END "; ## " FMT "\r\n", \
                       _FILENAME(__FILE__), __LINE__, __FUNCTION__, #EXP, ##__VA_ARGS__);                        \
            CONS_ABORT();                                                                                        \
        }                                                                                                        \
    } while (0)

/**
 * @brief Print data in hexadecima
 *
 * @param P point to data base
 * @param SIZE data size (bytes)
 * @param ALIGN word length, 1, 2, 4, 8
 * @param ENDIAN 0 -- little-endian, 1 -- big-endian
 */
#define SYS_LOG_DUMP(P, SIZE, ALIGN, ENDIAN)                                       \
    do                                                                             \
    {                                                                              \
        if (CONFIG_SYS_LOG_CONS_ON && CONFIG_SYS_LOG_DUMP_ON)                      \
        {                                                                          \
            _SYS_LOG_COLOR(1, "[HEX]:  " _GEN_DOMAIN, _COLOR_C, #P ": %d bytes%s", \
                           SIZE,                                                   \
                           ALIGN <= 1 ? "" : ENDIAN ? ", big-endian"               \
                                                    : ", little-endian");          \
            _sys_log_dump((unsigned)P, P, SIZE, ALIGN, ENDIAN);                    \
        }                                                                          \
    } while (0)

/**
 * @brief Print data in binary
 *
 * @param P point to data base
 * @param SIZE data size (bytes)
 * @param ALIGN word length, 1, 2, 4, 8
 * @param ENDIAN 0 -- little-endian, 1 -- big-endian
 */
#define SYS_LOG_BIN(P, SIZE, ALIGN, ENDIAN)                                        \
    do                                                                             \
    {                                                                              \
        if (CONFIG_SYS_LOG_CONS_ON && CONFIG_SYS_LOG_DUMP_ON)                      \
        {                                                                          \
            _SYS_LOG_COLOR(1, "[BIN]:  " _GEN_DOMAIN, _COLOR_C, #P ": %d bytes%s", \
                           SIZE,                                                   \
                           ALIGN <= 1 ? "" : ENDIAN ? ", big-endian"               \
                                                    : ", little-endian");          \
            _sys_log_bin((unsigned)P, P, SIZE, ALIGN, ENDIAN);                     \
        }                                                                          \
    } while (0)

    __static_inline void _sys_log_dump(unsigned addr,
                                       const void *p,
                                       int size,
                                       int width,
                                       int endian)
    {
#define BYTES_PRE_LINE 16
        if (size)
        {
            int i, ii, j;
            char buf[4 + 4 * BYTES_PRE_LINE];
            char *ascii;
            int len1;
            int len2;
            int lenmax;
            width = (width <= 1 ? 1 : (width <= 2 ? 2 : (width <= 4 ? 4 : 8)));
            size = (size + width - 1) / width;
            len1 = (1 + width * 2) * ((BYTES_PRE_LINE + width - 1) / width) - 1;
            len2 = 3 + (BYTES_PRE_LINE + width - 1) / width * width;
            lenmax = len1 + len2;
            ascii = &buf[len1 + 2];
            for (i = 0; i < 6 + sizeof(void *) * 2; i++)
            {
                SYS_PRINT("-");
            }
            for (i = 0; i < lenmax; i++)
            {
                buf[i] = '-';
            }
            buf[i] = '\0';
            SYS_PRINT("%s\r\n", buf);
            ii = (BYTES_PRE_LINE + width - 1) / width;
            while (size)
            {
                int index1 = 0;
                int index2 = 0;
                SYS_PRINT(_COLOR_Y "[" _COLOR_END "0x%08X" _COLOR_Y "]" _COLOR_END ": ", (unsigned)addr);
                for (i = 0; i < ii;)
                {
                    for (j = 0; j < width; j++)
                    {
                        if (endian)
                        {
                            index1 += SYS_SPRINT(&buf[index1], "%02x", ((uint8_t *)p)[j]);
                        }
                        else
                        {
                            index1 += SYS_SPRINT(&buf[index1], "%02x", ((uint8_t *)p)[width - j - 1]);
                        }
                        if (((uint8_t *)p)[j] >= ' ' && ((uint8_t *)p)[j] < 0x7f)
                        {
                            index2 += SYS_SPRINT(&ascii[index2], "%c", ((uint8_t *)p)[j]);
                        }
                        else
                        {
                            index2 += SYS_SPRINT(&ascii[index2], ".");
                        }
                    }
                    addr += width;
                    p = &((uint8_t *)p)[width];
                    if (--size > 0 && ++i < ii)
                    {
                        index1 += SYS_SPRINT(&buf[index1], " ");
                    }
                    else
                    {
                        while (index1 < len1)
                        {
                            buf[index1++] = ' ';
                        }
                        buf[index1++] = ' ';
                        buf[index1++] = '|';
                        index1 += index2;
                        while (index1 < lenmax - 1)
                        {
                            buf[index1++] = ' ';
                        }
                        buf[index1++] = '\0';
                        SYS_PRINT("%s|\r\n", buf);
                    }
                    if (size == 0)
                    {
                        for (i = 0; i < 6 + sizeof(void *) * 2; i++)
                        {
                            SYS_PRINT("=");
                        }
                        for (i = 0; i < lenmax; i++)
                        {
                            buf[i] = '=';
                        }
                        buf[i] = '\0';
                        SYS_PRINT("%s\r\n", buf);
                        break;
                    }
                }
            }
        }
#undef BYTES_PRE_LINE
    }

    __static_inline void _sys_log_bin(unsigned addr,
                                      const void *p,
                                      int size,
                                      int width,
                                      int endian)
    {
#define BITS_PRE_LINE 64
        if (size)
        {
            uint8_t value[8];
            int i, j;
            int lenmax;
            width = (width <= 1 ? 1 : (width <= 2 ? 2 : (width <= 4 ? 4 : 8)));
            width = width < BITS_PRE_LINE ? width : BITS_PRE_LINE;
            size = (size + width - 1) / width * width;
            lenmax = (1 + 3 * 8) * width;
            for (i = 0; i < 9 + sizeof(void *) * 2 + width * 2 + width * 8 / 4 + lenmax; i++)
            {
                SYS_PRINT("-");
            }
            SYS_PRINT("\r\n");
            for (i = 0; i < 8 + sizeof(void *) * 2 + width * 2; i++)
            {
                SYS_PRINT(" ");
            }
            for (i = 0; i < 8 * width; i++)
            {
                if (i % 4 == 0)
                {
                    if (i % 8 == 0)
                    {
                        SYS_PRINT(" ");
                    }
                    SYS_PRINT(" ");
                }
                SYS_PRINT("%3d", 8 * width - 1 - i);
            }
            SYS_PRINT("\r\n");
            for (i = 0; i < 9 + sizeof(void *) * 2 + width * 2 + width * 8 / 4 + lenmax; i++)
            {
                SYS_PRINT("~");
            }
            SYS_PRINT("\r\n");
            while (size)
            {
                SYS_PRINT(_COLOR_Y "[" _COLOR_END "0x%08X" _COLOR_Y "]" _COLOR_END ": 0x", (unsigned)addr);
                if (endian)
                {
                    for (i = 0; i < width; i++)
                    {
                        value[i] = ((uint8_t *)p)[i];
                        SYS_PRINT("%02X", value[i]);
                    }
                }
                else
                {
                    for (i = 0; i < width; i++)
                    {
                        value[i] = ((uint8_t *)p)[width - 1 - i];
                        SYS_PRINT("%02X", value[i]);
                    }
                }
                for (i = 0; i < width; i++)
                {
                    for (j = 0; j < 8; j++)
                    {
                        if (j % 4 == 0)
                        {
                            SYS_PRINT(" ");
                            if (j % 8 == 0)
                            {
                                SYS_PRINT(" ");
                            }
                        }
                        if (value[i] >> (7 - j) & 1)
                        {
                            SYS_PRINT("  1");
                        }
                        else
                        {
                            SYS_PRINT("  0");
                        }
                    }
                }
                SYS_PRINT("\n");
                addr += width;
                p = &((uint8_t *)p)[width];
                size -= width;
                if (size == 0)
                {
                    for (i = 0; i < 9 + sizeof(void *) * 2 + width * 2 + width * 8 / 4 + lenmax; i++)
                    {
                        SYS_PRINT("=");
                    }
                    SYS_PRINT("\r\n");
                }
            }
        }
#undef BITS_PRE_LINE
    }

#ifdef __cplusplus
}
#endif

#endif
