/**
 * @file sh_vt100.c
 * @author LokLiang (lokliang@163.com)
 * @brief
 * @version 0.1
 * @date 2023-03-21
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "sh.h"
#include "sh_vt100.h"
#include "sys_log.h"

#ifndef CONFIG_SH_MAX_HISTORY_LEN
#define CONFIG_SH_MAX_HISTORY_LEN 255 /* 用于记录命令的缓存长度 */
#endif

/**
 * @brief 定义热键列表 SH_DEF_KEY 中的成员
 *
 * @param KEY 热键代码（字符串）
 * @param FUNC 热键对应的函数( static void _key_func(void) )
 *
 * @verbatim
 * @c sh_vt100.c
 * @endverbatim
 */
#define SH_SETUP_KEY(KEY, FUNC) \
    {                           \
        .code = KEY,            \
        .key_func = FUNC,       \
    }

#define SH_DEF_KEY(NAME, ...)                       \
    static sys_psnode_t _SH_NAME(key_node_);        \
    static sh_key_t const _SH_NAME(key_data_)[] = { \
        __VA_ARGS__{0}};                            \
    static sh_key_reg_t const NAME = {              \
        .node = &_SH_NAME(key_node_),               \
        .key = _SH_NAME(key_data_),                 \
    };

sh_t g_uart_handle_vt100;

static char last_key = '\0';

/* 基本键键 */
static void _exec_key_TAB(sh_t *sh_hdl);       // 补全命令
static void _exec_key_ENTER(sh_t *sh_hdl);     // 执行命令
static void _exec_key_LF(sh_t *sh_hdl);        // 执行命令
static void _exec_key_BACKSPACE(sh_t *sh_hdl); // 删除光标前一个字符
static void _exec_key_CTR_H(sh_t *sh_hdl);     // 删除光标前一个字符
static void _exec_key_DELETE(sh_t *sh_hdl);    // 删除光标后一个字符
static void _exec_key_UP(sh_t *sh_hdl);        // 查看上一个命令
static void _exec_key_DW(sh_t *sh_hdl);        // 查看下一个命令
static void _exec_key_RIGHT(sh_t *sh_hdl);     // 光标右移
static void _exec_key_LEFT(sh_t *sh_hdl);      // 光标左移
static void _exec_key_HOME(sh_t *sh_hdl);      // 光标移到行首（提示符除外）
static void _exec_key_END(sh_t *sh_hdl);       // 光标移到行尾

/* 其他键键 */
static void _exec_key_CTR_C(sh_t *sh_hdl);     // 结束程序
static void _exec_key_CTR_D(sh_t *sh_hdl);     // 删除光标后一个字符或结束程序
static void _exec_key_CTR_L(sh_t *sh_hdl);     // 清空终端
static void _exec_key_CTR_A(sh_t *sh_hdl);     // 同 HOME
static void _exec_key_CTR_E(sh_t *sh_hdl);     // 同 END
static void _exec_key_CTR_U(sh_t *sh_hdl);     // 擦除光标前到行首的全部内容
static void _exec_key_CTR_K(sh_t *sh_hdl);     // 擦除光标后到行尾的全部内容
static void _exec_key_CTR_W(sh_t *sh_hdl);     // 擦除光标前的单词
static void _exec_key_CTR_Y(sh_t *sh_hdl);     // 还原擦除的内容
static void _exec_key_CTR_RIGHT(sh_t *sh_hdl); // 光标移动到单词首位
static void _exec_key_CTR_LEFT(sh_t *sh_hdl);  // 光标移动到单词结尾

/* 终端操作 */
static void _clear(sh_t *sh_hdl);                  // 清屏
static void _del_line(sh_t *sh_hdl, unsigned len); // 删除当前行
static void _set_cursor_hoffset(sh_t *sh_hdl, int last_pos, int new_pos);
static void _insert_str(sh_t *sh_hdl, const char *str);

SH_DEF_KEY(
    _register_vt100_keys,

    /* 基本 */
    SH_SETUP_KEY(KEY_TAB,       _exec_key_TAB),       // 补全命令
    SH_SETUP_KEY(KEY_CTR_M,     _exec_key_ENTER),     // 执行命令
    SH_SETUP_KEY(KEY_CTR_J,     _exec_key_LF),        // 执行命令
    SH_SETUP_KEY(KEY_BACKSPACE, _exec_key_BACKSPACE), // 删除光标前一个字符
    SH_SETUP_KEY(KEY_CTR_H,     _exec_key_CTR_H),     // 删除光标前一个字符
    SH_SETUP_KEY(KEY_DELETE,    _exec_key_DELETE),    // 删除光标后一个字符
    SH_SETUP_KEY(KEY_UP,        _exec_key_UP),        // 查看上一个命令
    SH_SETUP_KEY(KEY_DW,        _exec_key_DW),        // 查看下一个命令
    SH_SETUP_KEY(KEY_RIGHT,     _exec_key_RIGHT),     // 光标右移
    SH_SETUP_KEY(KEY_LEFT,      _exec_key_LEFT),      // 光标左移
    SH_SETUP_KEY(KEY_HOME,      _exec_key_HOME),      // 光标移到行首（提示符除外）
    SH_SETUP_KEY(KEY_END,       _exec_key_END),       // 光标移到行尾

    /* 其他 */
    SH_SETUP_KEY(KEY_CTR_C,     _exec_key_CTR_C),     // 结束程序
    SH_SETUP_KEY(KEY_CTR_D,     _exec_key_CTR_D),     // 删除光标后一个字符或结束程序
    SH_SETUP_KEY(KEY_CTR_L,     _exec_key_CTR_L),     // 清空终端
    SH_SETUP_KEY(KEY_CTR_A,     _exec_key_CTR_A),     // 同 HOME
    SH_SETUP_KEY(KEY_CTR_E,     _exec_key_CTR_E),     // 同 END
    SH_SETUP_KEY(KEY_CTR_U,     _exec_key_CTR_U),     // 擦除光标前到行首的全部内容
    SH_SETUP_KEY(KEY_CTR_K,     _exec_key_CTR_K),     // 擦除光标后到行尾的全部内容
    SH_SETUP_KEY(KEY_CTR_W,     _exec_key_CTR_W),     // 擦除光标前的单词
    SH_SETUP_KEY(KEY_CTR_Y,     _exec_key_CTR_Y),     // 还原擦除的内容
    SH_SETUP_KEY(KEY_CTR_RIGHT, _exec_key_CTR_RIGHT), // 光标移动到单词首位
    SH_SETUP_KEY(KEY_CTR_LEFT,  _exec_key_CTR_LEFT),  // 光标移动到单词结尾
);

static void _exec_key_TAB(sh_t *sh_hdl) // "\x09"
{
    sh_ctrl_tab(sh_hdl); // 自动实例/打印命令
}

static void _exec_key_ENTER(sh_t *sh_hdl) // "\x0D"
{
    sh_echo(sh_hdl, "\r\n");
    sh_ctrl_enter(sh_hdl); // 执行已缓存到的命令
    sh_ctrl_print_cmd_line(sh_hdl);
    last_key = '\r';
}

static void _exec_key_LF(sh_t *sh_hdl) // "\x0A"
{
    if (last_key != '\r')
    {
        _exec_key_ENTER(sh_hdl);
    }
    last_key = '\n';
}

static void _exec_key_DELETE(sh_t *sh_hdl) // "\e[3~"
{
    if (sh_ctrl_delete(sh_hdl, 1))
    {
        /* 删除光标右边一个字符 */
        sh_echo(sh_hdl, DCH(1));
    }
}

static void _exec_key_BACKSPACE(sh_t *sh_hdl) // "\x7F"
{
    if (sh_ctrl_backspace(sh_hdl, 1))
    {
        /* 删除光标左边一个字符，同时使光标自动左移一位 */
        sh_echo(sh_hdl, CUB(1));
        sh_echo(sh_hdl, DCH(1));
    }
}

static void _exec_key_CTR_H(sh_t *sh_hdl) // "\x8"
{
    if (sh_ctrl_backspace(sh_hdl, 1))
    {
        /* 删除光标左边一个字符，同时使光标自动左移一位 */
        uint16_t line_pos = sh_ctrl_get_line_pos(sh_hdl);
        uint16_t line_len = sh_ctrl_get_line_len(sh_hdl);

        sh_echo(sh_hdl, "\b");
        for (int i = 0; i < line_len - line_pos; i++)
        {
            sh_echo(sh_hdl, "%c", sh_ctrl_get_line_char(sh_hdl, line_pos + i));
        }
        sh_echo(sh_hdl, " ");
        for (int i = 0; i < line_len - line_pos + 1; i++)
        {
            sh_echo(sh_hdl, "\b");
        }
    }
}

static void _exec_key_RIGHT(sh_t *sh_hdl) // "\e[C"
{
    sh_ctrl_right(sh_hdl); // 光标右移
}

static void _exec_key_LEFT(sh_t *sh_hdl) // "\e[D"
{
    sh_ctrl_left(sh_hdl); // 光标左移
}

static void _exec_key_HOME(sh_t *sh_hdl) // "\e[H"
{
    sh_ctrl_home(sh_hdl); // 光标移到开头位置
}

static void _exec_key_END(sh_t *sh_hdl) // "\e[F"
{
    sh_ctrl_end(sh_hdl); // 光标移到结尾位置
}

static void _exec_key_UP(sh_t *sh_hdl) // "\e[A"
{
    int len = sh_ctrl_get_line_len(sh_hdl);
    if (sh_ctrl_up(sh_hdl)) // 上一个命令
    {
        _del_line(sh_hdl, len);
        sh_ctrl_print_cmd_line(sh_hdl);
    }
}

static void _exec_key_DW(sh_t *sh_hdl) // "\e[B"
{
    int len = sh_ctrl_get_line_len(sh_hdl);
    if (sh_ctrl_down(sh_hdl)) // 下一个命令
    {
        _del_line(sh_hdl, len);
        sh_ctrl_print_cmd_line(sh_hdl);
    }
}

__weak void drv_hal_sys_exit(void)
{
    SYS_LOG_WRN("never define exit function");
}
static void _exec_key_CTR_C(sh_t *sh_hdl) // "\x03"
{
    sh_echo(sh_hdl, "\r\n");
    drv_hal_sys_exit();
}

static void _exec_key_CTR_D(sh_t *sh_hdl) // "\x04"
{
    if (sh_ctrl_delete(sh_hdl, 1))
    {
        /* 删除光标右边一个字符 */
        sh_echo(sh_hdl, DCH(1));
    }
    else
    {
        uint16_t input_pos = sh_ctrl_get_input_pos(sh_hdl);
        if (input_pos == 0)
        {
            if (sh_ctrl_is_module(sh_hdl))
            {
                sh_putstr(sh_hdl, "exit\r");
            }
            else
            {
                drv_hal_sys_exit();
            }
        }
    }
}

static void _exec_key_CTR_L(sh_t *sh_hdl) // "\x0C"
{
    uint16_t input_pos = sh_ctrl_get_input_pos(sh_hdl);

    _clear(sh_hdl);
    _del_line(sh_hdl, sh_ctrl_get_line_len(sh_hdl));
    sh_ctrl_print_cmd_line(sh_hdl);

    sh_ctrl_set_input_pos(sh_hdl, input_pos);
}

static void _exec_key_CTR_A(sh_t *sh_hdl) // "\x01"
{
    sh_ctrl_home(sh_hdl); // 光标移到开头位置
}

static void _exec_key_CTR_E(sh_t *sh_hdl) // "\x05"
{
    sh_ctrl_end(sh_hdl); // 光标移到结尾位置
}

static void _exec_key_CTR_U(sh_t *sh_hdl) // "\x15"
{
    /* 删除光标左边的所有字符 */
    int n = sh_ctrl_del_left(sh_hdl);
    if (n)
    {
        sh_echo(sh_hdl, CUB(n));
        sh_echo(sh_hdl, DCH(n));
    }
}

static void _exec_key_CTR_K(sh_t *sh_hdl) // "\x0B"
{
    /* 删除光标右边的所有字符 */
    int n = sh_ctrl_del_right(sh_hdl);
    if (n)
    {
        sh_echo(sh_hdl, DCH(n));
    }
}

static void _exec_key_CTR_W(sh_t *sh_hdl) // "\x17"
{
    int n = sh_ctrl_del_word(sh_hdl);
    if (n)
    {
        /* 删除光标左边 n 个字符，同时使光标自动左移 n 位 */
        sh_echo(sh_hdl, CUB(n));
        sh_echo(sh_hdl, DCH(n));
    }
}

static void _exec_key_CTR_Y(sh_t *sh_hdl) // "\x19"
{
    sh_ctrl_undelete(sh_hdl);
}

static void _exec_key_CTR_RIGHT(sh_t *sh_hdl) // "\e[1;5C"
{
    sh_ctrl_word_tail(sh_hdl);
}

static void _exec_key_CTR_LEFT(sh_t *sh_hdl) // "\e[1;5D"
{
    sh_ctrl_word_head(sh_hdl);
}

static int _cmd_clear(sh_t *sh_hdl, int argc, const char *argv[])
{
    _clear(sh_hdl);
    return 0;
}
SH_DEF_CMD(
    _register_cmd_clear,
    SH_SETUP_CMD("clear", "Clear the terminal screen", _cmd_clear, NULL), // 清屏
);

#if defined(__linux__)

static void _clear(sh_t *sh_hdl)
{
    sh_echo(sh_hdl, "\ec");
}

static void _del_line(sh_t *sh_hdl, unsigned len)
{
    sh_echo(sh_hdl, DL(0));
}

static void _set_cursor_hoffset(sh_t *sh_hdl, int last_pos, int new_pos)
{
    /* 设置光标水平位置 */
    int hoffset = new_pos - last_pos;
    if (hoffset > 0) // 右移
    {
        sh_echo(sh_hdl, CUF(hoffset));
    }
    else if (hoffset < 0) // 左移
    {
        sh_echo(sh_hdl, CUB(-hoffset));
    }
}

#else

static void _clear(sh_t *sh_hdl)
{
    sh_echo(sh_hdl, "\ec");
}

static void _del_line(sh_t *sh_hdl, unsigned len)
{
    sh_echo(sh_hdl, "\r");
    for (int i = 0; i <= len; i++)
    {
        sh_echo(sh_hdl, " ");
    }
    sh_echo(sh_hdl, "\r");
}

static void _set_cursor_hoffset(sh_t *sh_hdl, int last_pos, int new_pos)
{
    /* 设置光标水平位置 */
    if (new_pos > last_pos) // 右移
    {
        for (; new_pos > last_pos; last_pos++)
        {
            char c = sh_ctrl_get_line_char(sh_hdl, last_pos);
            sh_echo(sh_hdl, "%c", c);
        }
    }
    else if (new_pos < last_pos) // 左移
    {
        for (; new_pos < last_pos; last_pos--)
        {
            sh_echo(sh_hdl, "\b");
        }
    }
}

#endif

static void _insert_str(sh_t *sh_hdl, const char *str)
{
    sh_echo(sh_hdl, ICH(strlen(str)));
    sh_echo(sh_hdl, "%s", str);
}

__used static void _clear_line(sh_t *sh_hdl)
{
    _del_line(sh_hdl, sh_ctrl_get_line_len(sh_hdl));
}

/**
 * @brief 初始化一个新的 VT100 的终端对象。
 *
 * @param sh_hdl 被初始化由 sh_putc() 或 sh_putstr() 的对象
 * @param vprint 注册实现函数 sh_hdl ==> 终端和显示
 * @param disconnect 当执行 exit 命令时被回调的，用于断开连接的函数。如不需要可设为 NULL
 */
int sh_init_vt100(sh_t *sh_hdl, sh_vprint_fn vprint, sh_disconnect_fn disconnect)
{
    int ret = 0;
    for (int i = 0; i < sizeof(*sh_hdl); i++)
    {
        ((uint8_t *)sh_hdl)[i] = 0;
    }

    sh_port_t port = {
        .set_cursor_hoffset = _set_cursor_hoffset,
        .insert_str = _insert_str,
        .vprint = vprint,
        .disconnect = disconnect,
        // .clear_line = _clear_line, // 执行清除一行。值为 NULL 时不启用刷命令行
    };
    sh_register_port(sh_hdl, &port);

    sh_hdl->cmd_buf = sh_hdl->cmd_line;

    sh_hdl->obj_key_data = _register_vt100_keys;
    sh_hdl->obj_key_data.node = &sh_hdl->obj_key_node;
    ret |= sh_register_key(sh_hdl, &sh_hdl->obj_key_data);

    sh_hdl->obj_cmd_data = _register_cmd_clear;
    sh_hdl->obj_cmd_data.node = &sh_hdl->obj_cmd_node;
    ret |= sh_register_key_cmd(sh_hdl, &sh_hdl->obj_cmd_data);

    return -!!ret;
}

static int _install_shell_vt100(void)
{
    if (sh_init_vt100(&g_uart_handle_vt100, SYS_VPRINT, NULL) == 0)
    {
        static uint8_t history_mem[CONFIG_SH_MAX_HISTORY_LEN];
        static uint8_t bank_mem[CONFIG_SH_MAX_LINE_LEN];
        sh_config_history_mem(&g_uart_handle_vt100, history_mem, sizeof(history_mem));
        sh_config_backup_mem(&g_uart_handle_vt100, bank_mem, sizeof(bank_mem));
        return 0;
    }
    else
    {
        return -1;
    }
}
INIT_EXPORT_COMPONENT(_install_shell_vt100);
