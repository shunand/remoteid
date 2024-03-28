/**
 * @file sh_vt100.h
 * @author LokLiang (lokliang@163.com)
 * @brief
 * @version 0.1
 * @date 2023-03-21
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __SH_VT100_h__
#define __SH_VT100_h__

/* 热键 */
#define KEY_CTR_A       "\x01"
#define KEY_CTR_B       "\x02"
#define KEY_CTR_C       "\x03"
#define KEY_CTR_D       "\x04"
#define KEY_CTR_E       "\x05"
#define KEY_CTR_F       "\x06"
#define KEY_CTR_G       "\x07"
#define KEY_CTR_H       "\x08"
#define KEY_CTR_I       "\x09"
#define KEY_TAB         "\x09"
#define KEY_CTR_J       "\x0A"
#define KEY_CTR_K       "\x0B"
#define KEY_CTR_L       "\x0C"
#define KEY_CTR_M       "\x0D"
#define KEY_CTR_N       "\x0E"
#define KEY_CTR_O       "\x0F"
#define KEY_CTR_P       "\x10"
#define KEY_CTR_Q       "\x11"
#define KEY_CTR_R       "\x12"
#define KEY_CTR_S       "\x13"
#define KEY_CTR_T       "\x14"
#define KEY_CTR_U       "\x15"
#define KEY_CTR_V       "\x16"
#define KEY_CTR_W       "\x17"
#define KEY_CTR_X       "\x18"
#define KEY_CTR_Y       "\x19"
#define KEY_CTR_Z       "\x1A"
#define KEY_PAUSE       "\x1A"
#define KEY_ESC         "\x1b"
#define KEY_BACKSPACE   "\x7F"
#define KEY_UP          "\x1b[A"
#define KEY_DW          "\x1b[B"
#define KEY_RIGHT       "\x1b[C"
#define KEY_LEFT        "\x1b[D"
#define KEY_HOME        "\x1b[H"
#define KEY_END         "\x1b[F"
#define KEY_CTR_UP      "\x1b[1;5A"
#define KEY_CTR_DW      "\x1b[1;5B"
#define KEY_CTR_RIGHT   "\x1b[1;5C"
#define KEY_CTR_LEFT    "\x1b[1;5D"
#define KEY_INSERT      "\x1b[2~"
#define KEY_DELETE      "\x1b[3~"
#define KEY_PAGE_UP     "\x1b[5~"
#define KEY_PAGE_DOWN   "\x1b[6~"
#define KEY_F1          "\x1bOP"
#define KEY_F2          "\x1bOQ"
#define KEY_F3          "\x1bOR"
#define KEY_F4          "\x1bOS"
#define KEY_F5          "\x1b[15~"
#define KEY_F6          "\x1b[17~"
#define KEY_F7          "\x1b[18~"
#define KEY_F8          "\x1b[19~"
#define KEY_F9          "\x1b[20~"
#define KEY_F10         "\x1b[21~"
#define KEY_F11         "\x1b[23~"
#define KEY_F12         "\x1b[24~"


/*光标操作符*/
#define CUU(n)      "\x1b[%dA",n        /* 光标向上       光标向上 <n> 行 */
#define CUD(n)      "\x1b[%dB",n        /* 光标向下       光标向下 <n> 行 */
#define CUF(n)      "\x1b[%dC",n        /* 光标向前       光标向前（右）<n> 行 */
#define CUB(n)      "\x1b[%dD",n        /* 光标向后       光标向后（左）<n> 行 */
#define CNL(n)      "\x1b[%dE",n        /* 光标下一行     光标从当前位置向下 <n> 行 */
#define CPL(n)      "\x1b[%dF",n        /* 光标当前行     光标从当前位置向上 <n> 行 */
#define CHA(n)      "\x1b[%dG",n        /* 绝对光标水平    光标在当前行中水平移动到第 <n> 个位置 */
#define VPA(n)      "\x1b[%dd",n        /* 绝对垂直行位置  光标在当前列中垂直移动到第 <n> 个位置 */
#define CUP(y,x)    "\x1b[%d;%dH",y,x   /* 光标位置       *光标移动到视区中的 <x>; <y> 坐标，其中 <x> 是 <y> 行的列 */
#define HVP(y,x)    "\x1b[%d;%df",y,x   /* 水平垂直位置   *光标移动到视区中的 <x>; <y> 坐标，其中 <x> 是 <y> 行的列 */

/*光标可见性*/
#define CU_START_BL "\x1b[?12h"    /* ATT160     文本光标启用闪烁    开始光标闪烁 */
#define CU_STOP_BL  "\x1b[?12l"    /* ATT160     文本光标禁用闪烁    停止闪烁光标 */
#define CU_SHOW     "\x1b[?25h"    /* DECTCEM    文本光标启用模式显示    显示光标 */
#define CU_HIDE     "\x1b[?25l"    /* DECTCEM    文本光标启用模式隐藏    隐藏光标 */

/* 字符操作 */
#define ICH(n)      "\x1b[%d@",n    /* 插入字符    在当前光标位置插入 <n> 个空格，这会将所有现有文本移到右侧。 向右溢出屏幕的文本会被删除。*/
#define DCH(n)      "\x1b[%dP",n    /* 删除字符    删除当前光标位置的 <n> 个字符，这会从屏幕右边缘以空格字符移动。*/
#define ECH(n)      "\x1b[%dX",n    /* 擦除字符    擦除当前光标位置的 <n> 个字符，方法是使用空格字符覆盖它们。*/
#define IL(n)       "\x1b[%dL",n    /* 插入行      将 <n> 行插入光标位置的缓冲区。 光标所在的行及其下方的行将向下移动。*/
#define DL(n)       "\x1b[%dM\r",n  /* 删除行      从缓冲区中删除 <n> 行，从光标所在的行开始。*/

/* 打印字体颜色设置 */
#define TX_DEF          "\x1b[0m"
#define TX_BLACK        "\x1b[30m"
#define TX_RED          "\x1b[31m"
#define TX_GREEN        "\x1b[32m"
#define TX_YELLOW       "\x1b[33m"
#define TX_BLUE         "\x1b[34m"
#define TX_WHITE        "\x1b[37m"

/* 打印背景颜色设置 */
#define BK_DEF          "\x1b[0m"
#define BK_BLACK        "\x1b[40m"
#define BK_RED          "\x1b[41m"
#define BK_GREEN        "\x1b[42m"
#define BK_YELLOW       "\x1b[43m"
#define BK_BLUE         "\x1b[44m"
#define BK_WHITE        "\x1b[47m"

#endif
