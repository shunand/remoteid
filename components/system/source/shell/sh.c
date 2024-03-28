/**
 * @file sh.c
 * @author LokLiang (lokliang@163.com)
 * @brief
 * @version 0.1
 * @date 2023-03-21
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "sh.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

#define SYS_LOG_DOMAIN "sh"
#include "sys_log.h"

#ifndef CONFIG_SH_MAX_PARAM
#define CONFIG_SH_MAX_PARAM (CONFIG_SH_MAX_LINE_LEN / 5) /* 以空格为分隔符的最多命令段数 */
#endif

#define _SH_ARRAY_COUNT(NAME) (sizeof(NAME) / sizeof(NAME[0]))
#define _IS_ALPHA(c) ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
#define _IS_DIGIT(c) (c >= '0' && c <= '9')

#define _YEAR ((((__DATE__[7] - '0') * 10 + (__DATE__[8] - '0')) * 10 + (__DATE__[9] - '0')) * 10 + (__DATE__[10] - '0'))

#define _MONTH ((__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n')   ? 1  \
                : (__DATE__[0] == 'F' && __DATE__[1] == 'e' && __DATE__[2] == 'b') ? 2  \
                : (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r') ? 3  \
                : (__DATE__[0] == 'A' && __DATE__[1] == 'p' && __DATE__[2] == 'r') ? 4  \
                : (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y') ? 5  \
                : (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n') ? 6  \
                : (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l') ? 7  \
                : (__DATE__[0] == 'A' && __DATE__[1] == 'u' && __DATE__[2] == 'g') ? 8  \
                : (__DATE__[0] == 'S' && __DATE__[1] == 'e' && __DATE__[2] == 'p') ? 9  \
                : (__DATE__[0] == 'O' && __DATE__[1] == 'c' && __DATE__[2] == 't') ? 10 \
                : (__DATE__[0] == 'N' && __DATE__[1] == 'o' && __DATE__[2] == 'v') ? 11 \
                                                                                   : 12)

#define _DAY ((__DATE__[4] == ' ' ? 0 : __DATE__[4] - '0') * 10 + (__DATE__[5] - '0'))

static sys_pslist_t s_cmd_list;      // 已注册的根命令链
static sys_pslist_t s_cmd_list_hide; // 已注册的根命令链（不会出现在补全中）

static const sh_key_t *_find_key(sh_t *sh_hdl, const char *code);                                                               // 在热键链中查找完全匹配的节点
static const sh_cmd_t *_find_cmd(sh_t *sh_hdl, const sh_cmd_reg_t **dest_reg_struct, const sh_cmd_t *sub_cmd, const char *cmd); // 在所有命令结构中中查找完全匹配的节点
static const sh_cmd_t *_find_root_cmd(sh_t *sh_hdl, const sh_cmd_reg_t **dest_reg_struct, const char *cmd);                     // 在根命令链中查找完全匹配的节点
static int _register_cmd(sh_t *sh_hdl, sys_pslist_t *list, const sh_cmd_reg_t *sh_reg);                                         // 注册命令
static int _register_cmd_check_fn(char *err_cmd, const sh_cmd_t *cmd);                                                          // 检查 sh_cmd_t::fn 的类型是否符合规则
static void _find_and_run_key(sh_t *sh_hdl, char c);                                                                            // 一系列动作：保存一个字符到热键缓存，并尝试查找对应的热键功能函数。缓存会被自动清除。
static void _insert_str(sh_t *sh_hdl, const char *str);                                                                         // 在当前的光标前插入一个字符并显示
static void _read_param(int *argc, const char *argv[CONFIG_SH_MAX_PARAM], char *dest, const char *src);                         // 从 src 中取得结果，输出到 int *argc, char **argv[], char *dest
static void _store_history(sh_t *sh_hdl, const char *src);                                                                      // 保存历史记录
static void _apply_history(sh_t *sh_hdl);                                                                                       // 当在回翻历史记录时，一旦对记录修改则立即将当前记录复制到缓存中
static unsigned _get_end_pos(sh_t *sh_hdl);                                                                                     // 根据当前显示内容，获取最大光标位置

static void _store_cmd_bak(sh_t *sh_hdl, const char *line, int pos, int n); // 保存擦除的数据
static void _clear_argv(int argc, const char *argv[]);                      // 使剩余的字符串指针指向空字符串

static const char *sh_ctrl_get_prompt(sh_t *sh_hdl); // 获取当前提示符
static const char *sh_ctrl_get_line(sh_t *sh_hdl);   // 获取当前已输入（或回显的历史）的字符串

static sh_cp_info_t _completion_init(const char *cmd); // 初始化待自动补全数据
static void _completion_update(
    sh_t *sh_hdl,
    sh_cp_info_t *info,
    const sh_cmd_t *sub_cmd,
    bool print_match,
    int max_lines,
    int flag_parent);
static sh_cp_op_t _sh_completion_cmd(sh_t *sh_hdl, int argc, const char *argv[], int flag_parent);
static bool _do_completion_cmd(sh_t *sh_hdl, sh_cp_info_t *info, const sh_cmd_info_t *cmd_info, bool print_match, int flag_parent); // 执行一次自动补全命令的功能
static bool _do_completion_param(sh_t *sh_hdl, sh_cp_info_t *info, const sh_cp_param_t *sh_param, bool print_match);                // 执行一次自动补全参数的功能
static bool _do_completion_insert(sh_t *sh_hdl, sh_cp_info_t *info);                                                                // 执行自动补全
static void _do_restore_line(sh_t *sh_hdl);                                                                                         // 在展示选项后恢复命令行的显示
static int _calc_list_algin(sh_cp_info_t *info);                                                                                    // 确定每个命令的对齐的长度
static bool _arg_is_end(sh_t *sh_hdl);                                                                                              // 当前输入的最后一个参数是否完整（以空格分开）

static void _update_cursor(sh_t *sh_hdl); // 同步热键功能：更新光标位置记录

#if defined(CONFIG_SH_USE_STRTOD) && (CONFIG_SH_USE_STRTOD == 1)
static const char *_skipwhite(const char *q);
static double _strtod(const char *str, char **end); // https://gitee.com/mirrors_mattn/strtod/blob/master/strtod.c
#endif

/**
 * @brief 查找由 sh_register_key() 注册的完全匹配的节点
 *
 * @param code 热键编码，@ref @c sh_vt100.h
 * @return const sh_key_t*
 */
static const sh_key_t *_find_key(sh_t *sh_hdl, const char *code)
{
    const sh_key_reg_t *test_reg_key;
    int match_count = 0;
    SYS_PSLIST_FOR_EACH_CONTAINER(&sh_hdl->key_list, test_reg_key, node)
    {
        const sh_key_t *test_key = test_reg_key->key;
        for (int i = 0; *(int *)&test_key[i] != 0; i++)
        {
            if (strcmp(test_key[i].code, code) == 0)
            {
                return &test_key[i];
            }
            if (strncmp(test_key[i].code, code, sh_hdl->key_stored) == 0)
            {
                match_count++;
            }
        }
    }

    if (match_count == 0)
    {
        sh_hdl->key_stored = 0;
        sh_hdl->key_str[sh_hdl->key_stored] = '\0';
    }
    return NULL;
}

/**
 * @brief 查找由 sh_register_cmd() sh_register_cmd_hide() sh_register_key_cmd() 注册的完全匹配的单元命令节点。
 * 如果当前为模块模式，由优先在模块中查找。
 *
 * @param dest_reg_struct[out] 保存根命令结构的定义地址
 * @param sub_cmd NULL -- 从根命令中历遍查找； ! NULL 从已知的父命令节点中查找
 * @param cmd 单元命令字符串
 * @return const sh_cmd_t* NULL -- 没有记录这个命令； ! NULL 找到的命令节点
 */
static const sh_cmd_t *_find_cmd(sh_t *sh_hdl, const sh_cmd_reg_t **dest_reg_struct, const sh_cmd_t *sub_cmd, const char *cmd)
{
    const sh_cmd_t *test_cmd;
    if (sh_hdl->select_cmd && sub_cmd == NULL)
    {
        test_cmd = sh_hdl->select_cmd->sub_fn.sub_cmd;
        for (int i = 0; test_cmd[i].cmd != NULL; i++)
        {
            if ((test_cmd->cmd_func != NULL || test_cmd->sub_fn.sub_cmd != NULL) &&
                strcmp(test_cmd[i].cmd, cmd) == 0)
            {
                if (dest_reg_struct)
                {
                    *dest_reg_struct = sh_hdl->select_reg_struct;
                }
                return &test_cmd[i];
            }
        }
    }

    if (sub_cmd == NULL)
    {
        if (dest_reg_struct)
        {
            *dest_reg_struct = NULL;
        }
        return _find_root_cmd(sh_hdl, dest_reg_struct, cmd);
    }
    else
    {
        test_cmd = sub_cmd;
        for (int i = 0; test_cmd[i].cmd != NULL; i++)
        {
            if ((test_cmd->cmd_func != NULL || test_cmd->sub_fn.sub_cmd != NULL) &&
                strcmp(test_cmd[i].cmd, cmd) == 0)
            {
                return &test_cmd[i];
            }
        }
        return NULL;
    }
}

/**
 * @brief 仅在根链表中查找由 sh_register_cmd() sh_register_cmd_hide() sh_register_key_cmd() 注册的完全匹配的单元命令节点。
 *
 * @param dest_reg_struct[out] 保存根命令结构的定义地址
 * @param cmd 单元命令字符串
 * @return const sh_cmd_t* NULL -- 没有记录这个命令； ! NULL 找到的命令节点
 */
static const sh_cmd_t *_find_root_cmd(sh_t *sh_hdl, const sh_cmd_reg_t **dest_reg_struct, const char *cmd)
{
    const sh_cmd_reg_t *sh_reg;
    sys_pslist_t *cmd_list[] = {
        &s_cmd_list_hide,
        &s_cmd_list,
        NULL,
    };
    if (sh_hdl)
    {
        cmd_list[sizeof(cmd_list) / sizeof(cmd_list[0]) - 1] = &sh_hdl->cmd_list;
    }
    for (int i = 0; i < sizeof(cmd_list) / sizeof(cmd_list[0]); i++)
    {
        sys_pslist_t *test_list = cmd_list[i];
        if (test_list != NULL)
        {
            SYS_PSLIST_FOR_EACH_CONTAINER(test_list, sh_reg, node)
            {
                const sh_cmd_t *reg_cmd = sh_reg->cmd;
                for (int i = 0; *(int *)&reg_cmd[i] != 0; i++)
                {
                    const sh_cmd_t *test_cmd = &reg_cmd[i];
                    if ((test_cmd->cmd_func != NULL || test_cmd->sub_fn.sub_cmd != NULL) &&
                        strcmp(test_cmd->cmd, cmd) == 0)
                    {
                        if (dest_reg_struct)
                        {
                            *dest_reg_struct = sh_reg;
                        }
                        return test_cmd;
                    }
                }
            }
        }
    }
    return NULL;
}

/**
 * @brief 执行注册根命令
 *
 * @param list 指定插入的链 @ref @c sh_register_cmd(),sh_register_cmd_hide(),sh_register_key_cmd()
 * @param sh_reg 根命令数据
 * @return int 0 -- 成功； ! 0 -- 命令中包含空格或命令已存在
 */
static int _register_cmd(sh_t *sh_hdl, sys_pslist_t *list, const sh_cmd_reg_t *sh_reg)
{
    const sh_cmd_t *reg_cmd = sh_reg->cmd;
    for (int i = 0; *(int *)&reg_cmd[i] != 0; i++)
    {
        const sh_cmd_t *test_cmd = &reg_cmd[i];
        char err_cmd[0x100];

        if (_register_cmd_check_fn(err_cmd, test_cmd))
        {
            SYS_LOG_WRN("sub_fn: pointer type mismatch in conditional expression!\r\n"
                        "Defined in file: %s:%d\r\n"
                        "Command:%s",
                        sh_reg->file,
                        sh_reg->line,
                        err_cmd);
            return -1;
        }

        if (strchr(test_cmd->cmd, ' '))
        {
            if (sh_hdl && sh_hdl->disable_echo == 0)
                SYS_LOG_WRN("Command cannot contain spaces: '%s'\r\n", test_cmd->cmd);
            return -1;
        }

        if (_find_root_cmd(sh_hdl, NULL, test_cmd->cmd))
        {
            if (sh_hdl && sh_hdl->disable_echo == 0)
                SYS_LOG_WRN("Repeat command: '%s'\r\n", test_cmd->cmd);
            return -1;
        }
    }

    sys_pslist_append(list, &sh_reg->node);

    return 0;
}

/**
 * @brief _register_cmd() 专用，递归遍历一个根命令及所有子命令，确认 sh_cmd_t::fn 的类型是否符合规则
 *
 * @param err_cmd[out]  输出记录错误的命令路径
 * @param cmd           命令
 * @retval 0 所有命令的 sh_cmd_t::fn 类型符合规则
 * @retval -1 有错误的 sh_cmd_t::fn 类型
 */
static int _register_cmd_check_fn(char *err_cmd, const sh_cmd_t *cmd)
{
    for (int i = 0; *(int *)&cmd[i] != 0; i++)
    {
        const sh_cmd_t *test_cmd = &cmd[i];
        strcpy(err_cmd, " ");
        strcpy(&err_cmd[1], test_cmd->cmd);

        if (test_cmd->cmd_func == NULL) // 需要子命令
        {
            if (test_cmd->fn_type == _SH_SUB_TYPE_SUB)
            {
                if (_register_cmd_check_fn(&err_cmd[strlen(err_cmd)], test_cmd->sub_fn.sub_cmd) != 0) // 递归查检子命令
                {
                    return -1;
                }
            }
            else
            {
                if (test_cmd->fn_type != _SH_SUB_TYPE_VOID || test_cmd->sub_fn.nul != NULL)
                {
                    return -1;
                }
            }
        }
        else // 是最终命令
        {
            if (test_cmd->fn_type != _SH_SUB_TYPE_CPFN)
            {
                if (test_cmd->fn_type != _SH_SUB_TYPE_VOID || test_cmd->sub_fn.nul != NULL)
                {
                    return -1;
                }
            }
        }
    }

    err_cmd[0] = '\0';
    return 0;
}

/**
 * @brief 保存一个字符到热键缓存，并尝试查找对应的热键功能函数。如果热代码错误缓存会被自动清除。
 *
 * @param c 字符流
 */
static void _find_and_run_key(sh_t *sh_hdl, char c)
{
    if (sh_hdl->key_stored < sizeof(sh_hdl->key_str) - 1)
    {
        sh_hdl->key_str[sh_hdl->key_stored++] = c;
        sh_hdl->key_str[sh_hdl->key_stored] = '\0';

        const sh_key_t *match_key = _find_key(sh_hdl, sh_hdl->key_str);
        if (match_key)
        {
            sh_hdl->key_stored = 0;
            sh_hdl->key_str[sh_hdl->key_stored] = '\0';
            match_key->key_func(sh_hdl);
            return;
        }
    }

    if (sh_hdl->key_stored >= sizeof(sh_hdl->key_str))
    {
        sh_hdl->key_stored = 0;
        sh_hdl->key_str[sh_hdl->key_stored] = '\0';
    }
}

/**
 * @brief 在当前的光标（指针）前插入并记录一个字符串。这个动作将同步到终端的命令行显示
 *
 * @param str 字符串流
 */
static void _insert_str(sh_t *sh_hdl, const char *str)
{
    int len = strlen(str);
    if (len)
    {
        int remain = sizeof(sh_hdl->cmd_line) - (sh_hdl->cmd_buf - sh_hdl->cmd_line) - sh_hdl->cmd_stored - 1;
        if (len > remain)
        {
            len = remain;
        }
        _apply_history(sh_hdl);
        if (sh_hdl->cmd_stored < remain)
        {
            if (sh_hdl->port.insert_str) /* 设置终端插入一个字符 */
            {
                sh_hdl->port.insert_str(sh_hdl, str);
            }

            for (int i = sh_hdl->cmd_stored - 1 + len; i > sh_hdl->cmd_input; i--) /* 使光标之后的数据往后移动 len 个字节 */
                sh_hdl->cmd_buf[i] = sh_hdl->cmd_buf[i - len];

            for (int i = 0; i < len; i++)
                sh_hdl->cmd_buf[sh_hdl->cmd_input + i] = str[i];

            sh_hdl->cmd_input += len;
            sh_hdl->cmd_stored += len;
            sh_hdl->sync_cursor += len;

            sh_hdl->cmd_buf[sh_hdl->cmd_stored] = '\0';

            /* 启用刷新命令行模式 */
            if (sh_hdl->port.clear_line)
            {
                sh_refresh_line(sh_hdl);
            }
        }
    }
}

/**
 * @brief 以空格为分隔符整理出参数。
 * 从 src 中取得结果，输出到 int *argc, char **argv[], char *dest
 *
 * @param argc[out] 保存参数个数
 * @param argv[out] 保存每个参数的地址（指向dest）
 * @param dest[out] 分离出来的参数字符串的副本，注意参数的使用期间需要保留。
 * @param src 已记录的命令行数据
 */
static void _read_param(int *argc, const char *argv[CONFIG_SH_MAX_PARAM], char *dest, const char *src)
{
    *argc = 0;
    while (*argc < CONFIG_SH_MAX_PARAM)
    {
        int len = 0;

        /* 以空格为分隔符，分段提取命令到 dest */
        while (*src == ' ')
        {
            ++src;
        }

        if (*src == '"') /* 使用双引号分隔的字符串 */
        {
            ++src;
            while (1)
            {
                dest[len] = src[len];
                if (dest[len] == '"')
                {
                    ++src;
                    dest[len] = '\0';
                }
                if (dest[len] == '\0')
                {
                    break;
                }

                if (dest[len] == '\\')
                {
                    ++src;
                    switch (src[len])
                    {
                    case '\0':
                        --src;
                        break;
                    case 'b':
                        dest[len] = '\b';
                        break;
                    case 'f':
                        dest[len] = '\f';
                        break;
                    case 'n':
                        dest[len] = '\n';
                        break;
                    case 'r':
                        dest[len] = '\r';
                        break;
                    case 't':
                        dest[len] = '\t';
                        break;
                    case '\"':
                    case '\\':
                    case '/':
                        dest[len] = src[len];
                        break;

                    default:
                        dest[len] = src[len];
                        break;
                    }
                }

                ++len;
            }
        }
        else
        {
            char split_char;
            if (*src == '\'') /* 使用单引号分隔的字符串 */
            {
                split_char = '\'';
                ++src;
            }
            else /* 使用空格分隔的字符串 */
            {
                split_char = ' ';
            }

            while (1)
            {
                dest[len] = src[len];
                if (dest[len] == split_char)
                {
                    ++src;
                    dest[len] = '\0';
                }
                if (dest[len] == '\0')
                {
                    break;
                }
                ++len;
            }
        }

        if (len) // dest 数据有效
        {
            argv[*argc] = dest;
            ++(*argc);

            dest = &dest[len + 1];
            src = &src[len];
        }
        else
        {
            break;
        }
    }
}

/**
 * @brief 尝试保存到历史记录中
 *
 * @param src 被保存的字符串（注意恢复空格）
 */
static void _store_history(sh_t *sh_hdl, const char *src)
{
    if (sh_hdl->disable_history == 0)
    {
        int len = strlen(src) + 1;
        int history_valid_num;

        if (*src == '\0')
        {
            return;
        }
        if (len > sh_hdl->history_size)
        {
            return;
        }

        if (strcmp(src, sh_hdl->cmd_history) != 0)
        {
            /* 更新历史信息 */
            for (int i = sh_hdl->history_index_num - 1; i > 0; i--)
                sh_hdl->history_index[i] = sh_hdl->history_index[i - 1] + len;

            /* 分析并刷新有效记录数 */
            for (history_valid_num = 0; history_valid_num < sh_hdl->history_index_num - 1; history_valid_num++)
            {
                int test_index = sh_hdl->history_index[history_valid_num];
                if (test_index > sh_hdl->history_size)
                {
                    --history_valid_num;
                    break;
                }
                if (sh_hdl->history_index[history_valid_num + 1] <= test_index)
                {
                    break;
                }
            }
            sh_hdl->history_valid_num = history_valid_num;

            /* 使历史数据往后移动 len 字节 */
            for (int i = sh_hdl->history_size - 1; i >= len; i--)
                sh_hdl->cmd_history[i] = sh_hdl->cmd_history[i - len];

            /* 复制新数据 */
            strcpy(sh_hdl->cmd_history, src);
        }
    }
}

/**
 * @brief 当在回翻历史记录时，一旦对记录修改则立即将当前记录复制到命令行的缓存中
 *
 * @param sh_hdl
 */
static void _apply_history(sh_t *sh_hdl)
{
    if (sh_hdl->history_show)
    {
        char *str = &sh_hdl->cmd_history[sh_hdl->history_index[sh_hdl->history_show - 1]];
        strcpy(sh_hdl->cmd_buf, str);
        sh_hdl->cmd_stored = strlen(str);
        sh_hdl->history_show = 0;
    }

    sh_hdl->tab_press_count = 0;
    sh_hdl->cp_resource_flag = 0;
}

/**
 * @brief 根据当前显示内容（包含回翻的历史记录），获取最大光标位置（不含提示符和模块提示）
 *
 * @return unsigned 结尾位置 0..n
 */
static unsigned _get_end_pos(sh_t *sh_hdl)
{
    if (sh_hdl->history_show)
        return strlen(&sh_hdl->cmd_history[sh_hdl->history_index[sh_hdl->history_show - 1]]);
    else
        return sh_hdl->cmd_stored;
}

/**
 * @brief 保存擦除的数据。
 * 保存最后一次因 KEY_CTR_U KEY_CTR_K 或 KEY_CTR_W 操作而被擦除的数据，
 * 用于 KEY_CTR_Y 还原的数据。如果长度不足则原来已保存的数据被清空
 *
 * @param line 当前完整的命令行数据
 * @param pos 被保存的位置 0..n
 * @param n 保存的长度
 */
static void _store_cmd_bak(sh_t *sh_hdl, const char *line, int pos, int n)
{
    if (sh_hdl->cmd_bank && sh_hdl->bank_size)
    {
        if (n > 0)
        {
            if (n < sh_hdl->bank_size - 1)
            {
                for (int i = 0; i < n; i++)
                {
                    sh_hdl->cmd_bank[i] = line[pos + i];
                }
                sh_hdl->cmd_bank[n] = '\0';
            }
            else
            {
                sh_hdl->cmd_bank[0] = '\0';
            }
        }
    }
}

/**
 * @brief 使剩余的字符串指针指向空字符串
 *
 * @param argc 参数数量
 * @param argv 参数字符串指针
 */
static void _clear_argv(int argc, const char *argv[])
{
    for (int i = argc; i < CONFIG_SH_MAX_PARAM; i++)
    {
        argv[i] = NULL;
    }
}

/**
 * @brief 获取当前提示符(不含模块提示符)
 *
 * @param sh_hdl
 * @return const char*
 */
static const char *sh_ctrl_get_prompt(sh_t *sh_hdl)
{
    if (sh_hdl->prompt)
        return sh_hdl->prompt;
    else
        return ">>> ";
}

/**
 * @brief 获取当前已输入（或回显的历史）的命令行数据
 *
 * @return const char* 指向历史记录或命令行缓存
 */
static const char *sh_ctrl_get_line(sh_t *sh_hdl)
{
    if (sh_hdl->history_show)
    {
        return &sh_hdl->cmd_history[sh_hdl->history_index[sh_hdl->history_show - 1]];
    }
    else
    {
        return sh_hdl->cmd_buf;
    }
}

/**
 * @brief 注册该对象的硬件抽象接口
 *
 * @param port 硬件接口的结构体（不要求静态类型）
 * @return int
 */
int sh_register_port(sh_t *sh_hdl, const sh_port_t *port)
{
    sh_hdl->port = *port;
    return -!port;
}

/**
 * @brief 注册根命令
 * @ref @c SH_REGISTER_CMD
 *
 * @param sh_reg 根命令数据
 * @return int 0 -- 成功； ! 0 -- 命令中包含空格或命令已存在
 */
int sh_register_cmd(const sh_cmd_reg_t *sh_reg)
{
    return _register_cmd(NULL, &s_cmd_list, sh_reg);
}

/**
 * @brief 注册根命令。这些根命令不会在补全和帮助中出现，但可被执行
 * @ref @c SH_REGISTER_CMD
 *
 * @param sh_reg 根命令数据
 * @return int 0 -- 成功； ! 0 -- 命令中包含空格或命令已存在
 */
int sh_register_cmd_hide(const sh_cmd_reg_t *sh_reg)
{
    return _register_cmd(NULL, &s_cmd_list_hide, sh_reg);
}

/**
 * @brief 取消注册一个根命令
 *
 * @param sh_reg 根命令数据
 * @return int 0 -- 成功； ! 0 -- 命令中包含空格或命令已存在
 */
int sh_unregister_cmd(const sh_cmd_reg_t *sh_reg)
{
    bool ret = (sh_reg == NULL);
    ret = (ret != false ? ret : sys_pslist_find_and_remove(&s_cmd_list, &sh_reg->node));
    ret = (ret != false ? ret : sys_pslist_find_and_remove(&s_cmd_list_hide, &sh_reg->node));
    return -!ret;
}

/**
 * @brief 注册一组热键
 * 适用于动态加载/卸载
 *
 * @param sh_key 热键数据
 * @return int 0 -- 成功； ! 0 -- 热键中包含空格或热键已存在
 */
int sh_register_key(sh_t *sh_hdl, const sh_key_reg_t *sh_key)
{
    if (_find_key(sh_hdl, sh_key->key->code))
    {
        if (sh_hdl->disable_echo == 0)
            SYS_LOG_WRN("Repeat key code: '%s'\r\n", sh_key->key->code);
        return -1;
    }

    sys_pslist_append(&sh_hdl->key_list, &sh_key->node);

    return 0;
}

/**
 * @brief 注册仅属于对应的终端可见的根命令。
 * @ref @c SH_REGISTER_CMD
 * 适用于动态加载/卸载
 *
 * @param sh_key 根命令数据
 * @return int 0 -- 成功； ! 0 -- 命令中包含空格或命令已存在
 */
int sh_register_key_cmd(sh_t *sh_hdl, const sh_cmd_reg_t *sh_reg)
{
    return _register_cmd(sh_hdl, &sh_hdl->cmd_list, sh_reg);
}

/**
 * @brief 取消注册一组热键
 * 适用于动态加载/卸载
 *
 * @param sh_key 热键数据
 * @return int 0 -- 成功； ! 0 -- 数据不存在
 */
int sh_unregister_key(sh_t *sh_hdl, const sh_key_reg_t *sh_key)
{
    return -!sys_pslist_find_and_remove(&sh_hdl->key_list, &sh_key->node);
}

/**
 * @brief 取消注册仅属于对应的终端可见的根命令
 * 适用于动态加载/卸载
 *
 * @param sh_key 根命令数据
 * @return int 0 -- 成功； ! 0 -- 数据不存在
 */
int sh_unregister_key_cmd(sh_t *sh_hdl, const sh_cmd_reg_t *sh_reg)
{
    return -!sys_pslist_find_and_remove(&sh_hdl->cmd_list, &sh_reg->node);
}

/**
 * @brief 配置用于记录历史的内存。sh_init_vt100() 默认不会配置此项。
 * @ref @c sh_vt100.c@_install_shell_vt100()
 *
 * @param mem 内存地址
 * @param size 内存大小（字节）
 */
void sh_config_history_mem(sh_t *sh_hdl, void *mem, int size)
{
    if (size > 10)
    {
        int rem = (sizeof(*sh_hdl->history_index) - (unsigned)mem % sizeof(*sh_hdl->history_index)) % sizeof(*sh_hdl->history_index);

        size -= rem;
        sh_hdl->history_index = (void *)((unsigned)mem + rem);

        int history_index_num = size / 10 + 1;
        if (history_index_num > (1 << sizeof(*sh_hdl->history_index) * 8) - 1)
        {
            history_index_num = (1 << sizeof(*sh_hdl->history_index) * 8) - 1;
        }
        sh_hdl->history_index_num = history_index_num;

        sh_hdl->cmd_history = (char *)&sh_hdl->history_index[sh_hdl->history_index_num];

        sh_hdl->history_size = size - sizeof(*sh_hdl->history_index) * sh_hdl->history_index_num;

        sh_hdl->history_valid_num = 0;
        sh_hdl->history_show = 0;
    }
}

/**
 * @brief 配置用于保存 sh_ctrl_del_word(), sh_ctrl_del_left(), sh_ctrl_del_right() 删除的内容。sh_init_vt100() 默认不会配置此项。
 * @ref @c sh_vt100.c@_install_shell_vt100()
 *
 * @param mem 内存地址
 * @param size 内存大小（字节）
 */
void sh_config_backup_mem(sh_t *sh_hdl, void *mem, int size)
{
    sh_hdl->cmd_bank = mem;
    sh_hdl->bank_size = size;
}

/**
 * @brief 输入字符流到内部缓存中并解析和执行。
 * 是整个数据解析流程的起点。
 *
 * @param sh_hdl 由 sh_init_vt100() 初始化的对象
 * @param c 字符流
 */
void sh_putc(sh_t *sh_hdl, char c)
{
    SYS_ASSERT(sh_hdl != NULL, "");

    if (sh_hdl->exec_flag == 0)
    {
        if (c < ' ' || c >= 0x7F || sh_hdl->key_stored > 0)
        {
            _find_and_run_key(sh_hdl, c);
            return;
        }
        else
        {
            char str[2] = {c, '\0'};
            _insert_str(sh_hdl, str);
        }
    }
}

/**
 * @brief 同 sh_putc()
 *
 * @param str 字符串流
 */
void sh_putstr(sh_t *sh_hdl, const char *str)
{
    SYS_ASSERT(sh_hdl != NULL, "");

    char buf[sizeof(sh_hdl->cmd_line)];
    const char *from = str;
    char *to = buf;
    for (int i = 0; i < sizeof(buf);)
    {
        char c = from[i];
        if (c < ' ' || c >= 0x7F || sh_hdl->key_stored > 0 || i >= sizeof(buf) - 1)
        {
            to[i++] = '\0';
            _insert_str(sh_hdl, to);

            if (c == '\0')
            {
                break;
            }
            else
            {
                sh_putc(sh_hdl, c);
                from = &from[i];
                to = &to[i];
                i = 0;
            }
        }
        else
        {
            to[i++] = c;
        }
    }
}

/**
 * @brief 同 sh_putstr() 但不会回显
 *
 * @param str 字符串流
 */
void sh_putstr_quiet(sh_t *sh_hdl, const char *str)
{
    SYS_ASSERT(sh_hdl != NULL, "");

    bool disable_echo = sh_hdl->disable_echo;
    bool disable_history = sh_hdl->disable_history;
    sh_port_t port_bak = sh_hdl->port;

    sh_hdl->disable_echo = true;
    sh_hdl->disable_history = true;
    sh_hdl->port.insert_str = NULL;
    sh_hdl->port.set_cursor_hoffset = NULL;
    sh_hdl->port.clear_line = NULL;

    sh_putstr(sh_hdl, str);

    sh_hdl->port = port_bak;
    sh_hdl->disable_history = disable_history;
    sh_hdl->disable_echo = disable_echo;
}

/**
 * @brief 设置提示符。
 *
 * @param prompt 提示符。要求为静态数据。
 */
void sh_set_prompt(sh_t *sh_hdl, const char *prompt)
{
    SYS_ASSERT(sh_hdl != NULL, "");

    sh_hdl->prompt = prompt;
}

/**
 * @brief 清除命令接收缓存（终端显示不会被更新）
 *
 */
void sh_reset_line(sh_t *sh_hdl)
{
    SYS_ASSERT(sh_hdl != NULL, "");

    sh_hdl->cmd_input = 0;
    sh_hdl->sync_cursor = 0;
    sh_hdl->cmd_stored = 0;
    sh_hdl->cmd_buf[sh_hdl->cmd_stored] = '\0';

    sh_hdl->key_stored = 0;
    sh_hdl->key_str[sh_hdl->key_stored] = '\0';

    sh_hdl->tab_press_count = 0;
    sh_hdl->cp_resource_flag = 0;
}

/**
 * @brief 回显到终端
 *
 */
int sh_echo(sh_t *sh_hdl, const char *fmt, ...)
{
    SYS_ASSERT(sh_hdl != NULL, "");

    if (sh_hdl != NULL && sh_hdl->disable_echo == 0)
    {
        if (sh_hdl->port.vprint)
        {
            va_list arg;
            va_start(arg, fmt);
            int ret = sh_hdl->port.vprint(fmt, arg);
            va_end(arg);
            return ret;
        }
    }

    return 0;
}

#if defined(CONFIG_SH_USE_STRTOD) && (CONFIG_SH_USE_STRTOD == 1)
/**
 * @brief _strtod() 用，跳过空格
 *
 * @param q 来源字符串
 * @return const char* 字符串首个非空格的地址
 */
static const char *_skipwhite(const char *q)
{
    const char *p = q;
    while (*p == ' ')
    {
        ++p;
    }
    return p;
}
/**
 * @brief 用于替换 <stdlib.h> 中的 strtod()
 */
static double _strtod(const char *str, char **end)
{
    double d = 0.0;
    int sign;
    int n = 0;
    const char *p, *a;

    a = p = str;
    p = _skipwhite(p);

    /* decimal part */
    sign = 1;
    if (*p == '-')
    {
        sign = -1;
        ++p;
    }
    else if (*p == '+')
    {
        ++p;
    }
    if (_IS_DIGIT(*p))
    {
        d = (double)(*p++ - '0');
        while (*p && _IS_DIGIT(*p))
        {
            d = d * 10.0 + (double)(*p - '0');
            ++p;
            ++n;
        }
        a = p;
    }
    else if (*p != '.')
    {
        goto done;
    }
    d *= sign;

    /* fraction part */
    if (*p == '.')
    {
        double f = 0.0;
        double base = 0.1;
        ++p;

        if (_IS_DIGIT(*p))
        {
            while (*p && _IS_DIGIT(*p))
            {
                f += base * (*p - '0');
                base /= 10.0;
                ++p;
                ++n;
            }
        }
        d += f * sign;
        a = p;
    }

    /* exponential part */
    if ((*p == 'E') || (*p == 'e'))
    {
        int e = 0;
        ++p;

        sign = 1;
        if (*p == '-')
        {
            sign = -1;
            ++p;
        }
        else if (*p == '+')
        {
            ++p;
        }

        if (_IS_DIGIT(*p))
        {
            while (*p == '0')
            {
                ++p;
            }

            if (*p == '\0')
            {
                --p;
            }
            e = (int)(*p++ - '0');
            while (*p && _IS_DIGIT(*p))
            {
                e = e * 10 + (int)(*p - '0');
                ++p;
            }
            e *= sign;
        }
        else if (!_IS_DIGIT(*(a - 1)))
        {
            a = str;
            goto done;
        }
        else if (*p == 0)
        {
            goto done;
        }

        if (d == 2.2250738585072011 && e == -308)
        {
            d = 0.0;
            a = p;
            errno = ERANGE;
            goto done;
        }
        if (d == 2.2250738585072012 && e <= -308)
        {
            d *= 1.0e-308;
            a = p;
            goto done;
        }
        d *= pow(10.0, (double)e);
        a = p;
    }
    else if (p > str && !_IS_DIGIT(*(p - 1)))
    {
        a = str;
        goto done;
    }

done:
    if (end)
    {
        *end = (char *)a;
    }
    return d;
}
#endif /* #if defined(CONFIG_SH_USE_STRTOD) && (CONFIG_SH_USE_STRTOD == 1) */

/**
 * @brief 参数工具：解析字符串对表示的值，包括 整数/浮点/字符串
 *
 * @param str 目标解析字符串
 * @return sh_parse_t 输出解析结果
 */
sh_parse_t sh_parse_value(const char *str)
{
#define _ATTR_OCT (1 << 0)
#define _ATTR_DEC (1 << 1)
#define _ATTR_HEX (1 << 2)
#define _ATTR_FLOAT (1 << 3)

    unsigned attr = 0;
    int dot = 0; // '.' 出现次数
    sh_parse_t ret = {
        .type = _PARSE_TYPE_STRING,
        .value.val_string = str,
        .endptr = (char *)str,
    };

    if (str == NULL || *str == '\0')
    {
        return ret;
    }

    ret.endptr = &((char *)str)[strlen(str)];

    /* 根据首个字符，初步确定属性值为 attr */
    do
    {
        if (*str == '+' || *str == '-')
        {
            str = &str[1];
        }

        if (*str == '0')
        {
            if (str[1] == 'x' || str[1] == 'X')
            {
                if ((str[2] >= '0' && str[2] <= '9') || (str[2] >= 'a' && str[2] <= 'f') || (str[2] >= 'A' && str[2] <= 'F'))
                {
                    attr = _ATTR_HEX;
                    str = &str[3];
                }
                else
                {
                    return ret;
                }
            }
            else
            {
                attr = _ATTR_OCT | _ATTR_FLOAT;
            }
        }
        else if (*str == '.')
        {
            attr = _ATTR_FLOAT;
            str = &str[1];
            dot++;
            if (*str == '\0')
            {
                return ret;
            }
        }
        else if (*str >= '1' && *str <= '9')
        {
            attr = _ATTR_DEC | _ATTR_FLOAT;
        }

    } while (0);

    /* 按照 attr 给出的可能性，按具体特性单独进行格式检测，若格式正确则进行数值解析并返回 */
    do
    {
        if (attr & _ATTR_OCT) /* 八进制 */
        {
            while (*str >= '0' && *str <= '7')
            {
                str = &str[1];
            }

            if (*str == '\0')
            {
                if (*ret.value.val_string == '-')
                {
                    ret.type = _PARSE_TYPE_INTEGER;
                    ret.value.val_integer = strtol(ret.value.val_string, &ret.endptr, 8);
                }
                else
                {
                    ret.type = _PARSE_TYPE_UNSIGNED;
                    ret.value.val_unsigned = strtoul(ret.value.val_string, &ret.endptr, 8);
                }
                return ret;
            }
        }

        if (attr & _ATTR_DEC) /* 十进制 */
        {
            while (*str >= '0' && *str <= '9')
            {
                str = &str[1];
            }

            if (*str == '\0')
            {
                if (*ret.value.val_string == '-')
                {
                    ret.type = _PARSE_TYPE_INTEGER;
                    ret.value.val_integer = strtol(ret.value.val_string, &ret.endptr, 10);
                }
                else
                {
                    ret.type = _PARSE_TYPE_UNSIGNED;
                    ret.value.val_unsigned = strtoul(ret.value.val_string, &ret.endptr, 10);
                }
                return ret;
            }
        }

        if (attr & _ATTR_HEX) /* 十六进制 */
        {
            while ((*str >= '0' && *str <= '9') || (*str >= 'a' && *str <= 'f') || (*str >= 'A' && *str <= 'F'))
            {
                str = &str[1];
            }

            if (*str == '\0')
            {
                if (*ret.value.val_string == '-')
                {
                    ret.type = _PARSE_TYPE_INTEGER;
                    ret.value.val_integer = strtol(ret.value.val_string, &ret.endptr, 16);
                }
                else
                {
                    ret.type = _PARSE_TYPE_UNSIGNED;
                    ret.value.val_unsigned = strtoul(ret.value.val_string, &ret.endptr, 16);
                }
                return ret;
            }
        }

#if defined(CONFIG_SH_USE_STRTOD) && (CONFIG_SH_USE_STRTOD == 1)
        if (attr & _ATTR_FLOAT) /* 浮点数 */
        {
            int e = 0; // 'e' 出现次数

            while (dot < 2 && e < 2)
            {
                while (*str >= '0' && *str <= '9')
                {
                    str = &str[1];
                }

                if (*str == '\0')
                {
                    if (attr & _ATTR_OCT)
                    {
                        if (dot + e == 0)
                        {
                            return ret; /* 如 08、09 */
                        }
                    }
                    ret.type = _PARSE_TYPE_FLOAT;
                    ret.value.val_float = _strtod(ret.value.val_string, &ret.endptr);
                    return ret;
                }

                switch (*str)
                {
                case '.':
                {
                    dot++;
                    str = &str[1];
                    break;
                }
                case 'e':
                {
                    e++;
                    str = &str[1];
                    if (*str == '\0')
                    {
                        return ret;
                    }
                    else if (*str == '+' || *str == '-')
                    {
                        str = &str[1];
                        if (*str == '\0')
                        {
                            return ret;
                        }
                    }
                    break;
                }
                default:
                    return ret;
                }
            }
        }
#endif /* #if defined(CONFIG_SH_USE_STRTOD) && (CONFIG_SH_USE_STRTOD == 1) */

    } while (0);

    return ret;
}

/**
 * @brief 参数工具：以空格为分隔符，合并多个参数
 *
 * @param dst 用于保存合并结果的内存地址
 * @param len 用于保存合并结果的内存长度
 * @param argc 参数数量
 * @param argv 参数字符串指针
 * @return int 合并后的长度（不包含结束字符）
 */
int sh_merge_param(char *dst, int len, int argc, const char *argv[])
{
    int ret = 0;
    if (len)
    {
        char *p = dst;
        for (int i = 0; i < argc; i++)
        {
            size_t n = strlen(argv[i]) + 1;

            if (n > len)
                n = len;

            strncpy(p, argv[i], n);

            if (i + 1 < argc)
                p[n - 1] = ' ';
            else
                p[n - 1] = '\0';

            p = &p[n];
            len -= n;
            ret += n;
            if (len == 0)
            {
                break;
            }
        }
        --ret;
    }
    return ret;
}

/**
 * @brief 获取上一次命令的执行结果
 *
 * @param sh_hdl 由 sh_init_vt100() 初始化的对象
 * @retval 1 command not found
 * @retval 2 command not complete
 * @retval 其他值：由被执行的命令的返回值
 */
int sh_get_cmd_result(sh_t *sh_hdl)
{
    SYS_ASSERT(sh_hdl != NULL, "");

    return sh_hdl->cmd_return;
}

/**
 * @brief 执行刷一次当前命令行显示
 * 注意仅在 sh_port_t::clear_line 被设置时有效
 */
void sh_refresh_line(sh_t *sh_hdl)
{
    SYS_ASSERT(sh_hdl != NULL, "");

    if (sh_hdl->port.clear_line)
    {
        uint16_t input_pos = sh_hdl->cmd_input;
        sh_hdl->port.clear_line(sh_hdl);
        sh_ctrl_print_cmd_line(sh_hdl);
        sh_ctrl_set_input_pos(sh_hdl, input_pos);
    }
}

/**
 * @brief 自动补全功能用：初始化补全命令信息。
 *
 * @param cmd 待解析的实例命令字符串
 * @return sh_cp_info_t 输出初始的基本补全信息
 */
static sh_cp_info_t _completion_init(const char *cmd)
{
    sh_cp_info_t info = {
        .arg_str = cmd,         // 待分析的参数
        .arg_len = strlen(cmd), // 待解析命令长度
        .match_num = 0,         // 可打印的列表的条目数
        .list_algin = 0,        // 确定格式中 %-*s 的 * 的值
    };
    info.match_str[0] = '\0'; // 保存相同的字符部分
    return info;
}

/**
 * @brief 自动补全功能用：每被执行一次都被更新一次补全信息或直接打印可选项。
 *
 * @param info[out] 由 _completion_init() 初始化的补全信息指针，并被实时记录刷新。
 * @param sub_cmd 包含所有可选项的父节点。
 * @param print_match false -- 只刷新 info; true -- 只打印匹配的备选项。
 * @param max_lines print_match 为 true 时，可打印的选项数超过这个设定时，仅列出匹配的命令（不打印帮助信息）。
 * @param flag_parent 仅在内部的命令 select 中置 1 使用，表示只包含父命令的可选项。
 */
static void _completion_update(sh_t *sh_hdl, sh_cp_info_t *info, const sh_cmd_t *sub_cmd, bool print_match, int max_lines, int flag_parent)
{
    int list_algin = 100 / _calc_list_algin(info);

    if ((sub_cmd->cmd_func != NULL || sub_cmd->sub_fn.sub_cmd != NULL) &&
        (info->arg_len == 0 || strncmp(sub_cmd->cmd, info->arg_str, info->arg_len) == 0))
    {
        if (flag_parent && sub_cmd->cmd_func) // 只按父命令更新
        {
            return;
        }

        if (print_match == false) // 更新补全信息
        {
            int test_len = strlen(sub_cmd->cmd);
            if (info->list_algin < test_len)
                info->list_algin = test_len;

            if (info->match_num)
            {
                for (int i = 0; info->match_str[i] != '\0'; i++)
                {
                    if (info->match_str[i] != sub_cmd->cmd[i])
                    {
                        info->match_str[i] = '\0';
                        break;
                    }
                }
            }
            else
            {
                strcpy(info->match_str, sub_cmd->cmd);
            }

            info->match_num++;
        }
        else // 打印匹配的命令
        {
            info->print_count++;

            const char *color_begin = "";
            const char *color_end = "";
            if (sub_cmd->cmd_func)
            {
                if (sub_cmd->sub_fn.cp_fn) // 有参数
                {
                    color_begin = _COLOR_C;
                    color_end = _COLOR_END;
                }
            }
            else // 有子命令
            {
                color_begin = _COLOR_P;
                color_end = _COLOR_END;
            }
            sh_echo(sh_hdl, "%s%-*s%s", color_begin, _calc_list_algin(info), sub_cmd->cmd, color_end);

            if (info->match_num > max_lines) // 超过 max_lines 个匹配项时，不再显示帮助信息
            {
                if (info->print_count >= info->match_num || info->print_count % list_algin == 0)
                {
                    sh_echo(sh_hdl, "\r\n");
                }
            }
            else
            {
                if (sub_cmd->help && *sub_cmd->help != '\0')
                {
                    sh_echo(sh_hdl, "-- %s\r\n", sub_cmd->help);
                }
                else
                {
                    sh_echo(sh_hdl, "\r\n");
                }
            }
        }
    }
}

/**
 * @brief 自动补全功能用：执行一次自动补全命令或打印备选命令的功能
 *
 * @param info[out] 由 _completion_init() 初始化的补全信息指针，并被实时记录刷新。
 * @param cmd_info 包含所有备选项的父节点的原始数据结构。
 * @param print_match false -- 仅尝试自动补全命令; true -- 只打印匹配的备选项。
 * @param flag_parent 仅在内部的命令 select 中置 1 使用，表示只包含父命令的备选命令。
 * @return true 当 print_match 为 false （仅尝试自动补全命令）时，成功执行了一次补全命令的动作
 * @return false 当 print_match 为 true （只打印匹配的备选项）或无法补全命令
 */
static bool _do_completion_cmd(sh_t *sh_hdl, sh_cp_info_t *info, const sh_cmd_info_t *cmd_info, bool print_match, int flag_parent)
{
    char new_cmd[sizeof(sh_hdl->cmd_line)];
    const sh_cmd_t *sub_cmd;
    int err_status;

    if (print_match)
    {
        if (info->match_num == 0)
        {
            return false;
        }

        if (sh_hdl->cp_operate != SH_CP_OP_PEND)
        {
            sh_hdl->cp_operate = SH_CP_OP_PEND;
            sh_echo(sh_hdl, "\r\n");
        }
        info->print_count = 0;
    }
    else
    {
        info->match_num = 0;
    }

    if (cmd_info)
    {
        sub_cmd = cmd_info->match_cmd;
        err_status = cmd_info->err_status;
    }
    else
    {
        sub_cmd = sh_hdl->select_cmd;
        err_status = !_SH_CMD_STATUS_Incomplete;
    }
    if (sub_cmd == NULL)
    {
        sub_cmd = sh_hdl->select_cmd;
    }

    while (1)
    {
        if (sub_cmd == NULL) // 历遍根命令链
        {
            const sh_cmd_reg_t *sh_reg;
            sys_pslist_t *cmd_list[] = {
                &s_cmd_list,
                &sh_hdl->cmd_list,
            };
            for (int i = 0; i < sizeof(cmd_list) / sizeof(cmd_list[0]); i++)
            {
                sys_pslist_t *test_list = cmd_list[i];
                SYS_PSLIST_FOR_EACH_CONTAINER(test_list, sh_reg, node)
                {
                    const sh_cmd_t *reg_cmd = sh_reg->cmd;
                    for (int i = 0; *(int *)&reg_cmd[i] != 0; i++)
                    {
                        sh_cmd_t new_data = reg_cmd[i];
                        new_data.cmd = new_cmd;
                        strcpy(new_cmd, reg_cmd[i].cmd);
                        strcat(new_cmd, " ");
                        _completion_update(sh_hdl, info, &new_data, print_match, 30, flag_parent);
                    }
                }
            }
        }
        else // 历遍子命令数据
        {
            sub_cmd = sub_cmd->sub_fn.sub_cmd;
            for (int i = 0; sub_cmd[i].cmd != NULL; i++)
            {
                sh_cmd_t new_data = sub_cmd[i];
                new_data.cmd = new_cmd;
                strcpy(new_cmd, sub_cmd[i].cmd);
                strcat(new_cmd, " ");
                _completion_update(sh_hdl, info, &new_data, print_match, 30, flag_parent);
            }

            if (sh_hdl->select_cmd != NULL &&              // 当前状态为模块模式
                info->print_count == 0 &&                  // 没有打印任何可选项
                err_status != _SH_CMD_STATUS_Incomplete && // 如果正在输出当前模块下的子命令，则只继续补全子命令
                print_match == true &&                     // 只打印匹配的备选项
                flag_parent == 0                           // 目前仅在 _cmd_select_param() 被置起，此时不再历遍根命令链
            )
            {
                sub_cmd = NULL; // 使从根命令链重新查找
                continue;
            }
        }

        if (sh_hdl->select_cmd != NULL &&              // 当前状态为模块模式
            info->match_num == 0 &&                    // 没有找到可匹配的命令
            err_status != _SH_CMD_STATUS_Incomplete && // 如果正在输出当前模块下的子命令，则只继续补全子命令
            sub_cmd != NULL                            // 未历遍过根命令链
        )
        {
            sub_cmd = NULL; // 使从根命令链重新查找
        }
        else
        {
            break;
        }
    }

    if (print_match)
    {
        return false;
    }

    return _do_completion_insert(sh_hdl, info);
}

/**
 * @brief 自动补全功能用：执行一次自动补全参数或打印备选参数的功能
 *
 * @param info[out] 由 _completion_init() 初始化的补全信息指针，并被实时记录刷新。
 * @param sh_param 包含所有备选项的参数的结构。注意结构体必须有值为 NULL 的成员为结束。
 * @param print_match false -- 仅尝试自动补全参数; true -- 只打印匹配的备选项。
 * @return true 当 print_match 为 false （仅尝试自动补全参数）时，成功执行了一次补全参数的动作
 * @return false 当 print_match 为 true （只打印匹配的备选项）或无法补全参数
 */
static bool _do_completion_param(sh_t *sh_hdl, sh_cp_info_t *info, const sh_cp_param_t *sh_param, bool print_match)
{
    if (print_match)
    {
        if (info->match_num == 0)
        {
            return false;
        }

        if (sh_hdl->cp_operate != SH_CP_OP_PEND)
        {
            sh_echo(sh_hdl, "\r\n");
            sh_hdl->cp_operate = SH_CP_OP_PEND;
        }
        info->print_count = 0;
    }
    else
    {
        info->match_num = 0;
    }

    for (int i = 0; sh_param[i].cmd != NULL; i++)
    {
        char new_cmd[sizeof(sh_hdl->cmd_line)];
        sh_cmd_t sub_cmd = {
            .cmd = new_cmd,
            .help = sh_param[i].help,
            .cmd_func = (__typeof(sub_cmd.cmd_func))1,
            .sub_fn = {.sub_cmd = NULL},
        };
        strcpy(new_cmd, sh_param[i].cmd);
        strcat(new_cmd, " ");
        _completion_update(sh_hdl, info, &sub_cmd, print_match, 30, 0);
    }

    if (print_match)
    {
        return false;
    }

    return _do_completion_insert(sh_hdl, info);
}

/**
 * @brief 自动补全功能用：执行补全
 *
 * @param info[out] 由 _completion_init() 初始化的补全信息指针
 * @return true 成功执行了一次补全的动作
 * @return false 未执行补全的动作
 */
static bool _do_completion_insert(sh_t *sh_hdl, sh_cp_info_t *info)
{
    /* 执行自动补全，返回：显示是否被自动更新 */
    if (info->match_str[info->arg_len] == '\0')
    {
        return false;
    }
    else
    {
        _insert_str(sh_hdl, &info->match_str[info->arg_len]);
        sh_hdl->cp_operate = SH_CP_OP_CP;
        return true;
    }
}

/**
 * @brief 在展示备选项后恢复命令行的显示
 */
static void _do_restore_line(sh_t *sh_hdl)
{
    if (sh_hdl->cp_operate == SH_CP_OP_PEND)
    {
        uint16_t cmd_input = sh_hdl->cmd_input;
        sh_ctrl_print_cmd_line(sh_hdl);
        sh_ctrl_set_input_pos(sh_hdl, cmd_input);
        sh_hdl->cp_operate = SH_CP_OP_LIST;
    }
}

/**
 * @brief 确定每个命令或参数的对齐的长度
 *
 * @param info[out] 由 _completion_update() 更新的补全信息指针
 * @return int 每个命令或参数应占用的打印空间
 */
static int _calc_list_algin(sh_cp_info_t *info)
{
    return (info->list_algin > 8 ? info->list_algin : 8) + 2;
}

/**
 * @brief 完整解析命令并获取命令信息
 *
 * @param argc 命令数量
 * @param argv 命令字符串指针
 * @return sh_cmd_info_t 命令信息
 */
sh_cmd_info_t sh_cmd_info(sh_t *sh_hdl, int argc, const char *argv[])
{
    SYS_ASSERT(sh_hdl != NULL, "");

    unsigned cmd_sections = 0;
    sh_cmd_info_t info;

    info.err_status = _SH_CMD_STATUS_Bad;
    info.match_count = 0;
    info.reg_struct = NULL;
    info.match_cmd = NULL;

    for (cmd_sections = 0; cmd_sections < argc; cmd_sections++) // 依次查找全部子命令
    {
        info.match_cmd = _find_cmd(sh_hdl, &info.reg_struct, info.match_cmd, argv[cmd_sections]);

        if (info.match_cmd == NULL)
        {
            info.err_status = _SH_CMD_STATUS_Bad;
            info.match_count = cmd_sections;
            return info;
        }

        if (info.match_cmd->cmd_func) // 找到子命令
        {
            info.err_status = _SH_CMD_STATUS_Success;
            info.match_count = cmd_sections + 1;
            return info;
        }

        if (cmd_sections + 1 < argc)
        {
            info.match_cmd = info.match_cmd->sub_fn.sub_cmd;
        }
    }

    if (info.match_cmd && info.match_cmd->sub_fn.sub_cmd && cmd_sections == argc)
    {
        info.err_status = _SH_CMD_STATUS_Incomplete;
        info.match_count = cmd_sections;
        return info;
    }

    return info;
}

/**
 * @brief 根据输入的参数，结合当前命令行的状态，在已注册命令中查找并自执行一次自动补全命令/打印备选命令的全过程
 *
 * @param argc 命令数量
 * @param argv 命令字符串指针
 * @param flag_parent 仅在内部的命令 select 中置 1 使用，表示只包含父命令的可选项。
 * @return sh_cp_op_t 自动补全命令/打印备选命令的执行结果
 */
static sh_cp_op_t _sh_completion_cmd(sh_t *sh_hdl, int argc, const char *argv[], int flag_parent)
{
    sh_cmd_info_t cmd_info;
    sh_cp_info_t cp_info;

    if (sh_hdl->exec_flag == 0)
    {
        sh_hdl->cp_operate = SH_CP_OP_NA;
    }

    if (argc)
    {
        /* 初始化 cmd_info 和 cp_info */
        if (_arg_is_end(sh_hdl))
        {
            cmd_info = sh_cmd_info(sh_hdl, argc, argv);
            cp_info = _completion_init("");
        }
        else
        {
            cmd_info = sh_cmd_info(sh_hdl, --argc, argv);
            if (cmd_info.err_status == _SH_CMD_STATUS_Success)
            {
                cp_info = _completion_init(argv[argc++]);
            }
            else
            {
                cp_info = _completion_init(argv[cmd_info.match_count]);
            }
        }

        if (cmd_info.err_status == _SH_CMD_STATUS_Success)
        {
            if (cmd_info.match_cmd->sub_fn.cp_fn != NULL)
            {
                do
                {
                    if (sh_hdl->cp_resource_flag) // 仅作用于使用了 sh_completion_resource() 的情况
                    {
                        if (sh_hdl->tab_press_count)
                        {
                            cp_info.match_num = sh_hdl->cp_match_num;
                            cp_info.list_algin = sh_hdl->cp_list_algin;
                            if (cp_info.match_num < 2)
                            {
                                break;
                            }

                            if (sh_hdl->cp_operate != SH_CP_OP_PEND)
                            {
                                sh_hdl->cp_operate = SH_CP_OP_PEND;
                                sh_echo(sh_hdl, "\r\n");
                            }
                        }
                    }

                    ++sh_hdl->exec_flag;
                    sh_hdl->cp_info = &cp_info;

                    _clear_argv(argc, argv);
                    cmd_info.match_cmd->sub_fn.cp_fn(sh_hdl, argc - cmd_info.match_count, &argv[cmd_info.match_count], _arg_is_end(sh_hdl));

                    sh_hdl->cp_info = NULL;
                    --sh_hdl->exec_flag;

                    if (sh_hdl->cp_resource_flag) // 仅作用于使用了 sh_completion_resource() 的情况
                    {
                        int cp_resource_flag = sh_hdl->cp_resource_flag;
                        if (sh_hdl->tab_press_count == 0)
                        {
                            if (cp_info.match_num)
                            {
                                sh_hdl->tab_press_count = 1;
                                sh_hdl->cp_match_num = cp_info.match_num;
                                sh_hdl->cp_list_algin = cp_info.list_algin;
                                _do_completion_insert(sh_hdl, &cp_info);
                            }
                        }
                        else
                        {
                            /* 恢复当前命令行显示 */
                            _do_restore_line(sh_hdl);
                        }
                        sh_hdl->cp_resource_flag = cp_resource_flag;
                    }

                } while (0);
            }

            return sh_hdl->cp_operate; // 包含完整的命令
        }

        if (cmd_info.match_count != argc)
        {
            return sh_hdl->cp_operate; // 中间的命令错误
        }
    }
    else
    {
        cmd_info = sh_cmd_info(sh_hdl, argc, argv);
        cp_info = _completion_init(""); // 没有任何命令时直接打印所有根命令
    }

    do
    {
        if (_do_completion_cmd(sh_hdl, &cp_info, &cmd_info, false, flag_parent))
        {
            break; // 因为命令行被自动更新，不打印可用命令
        }
        if (cp_info.match_num == 0)
        {
            break; // 没有匹配的命令
        }
        if (sh_hdl->tab_press_count == 0)
        {
            sh_hdl->tab_press_count = 1;
            break; // 暂停一次操作
        }

        /* 打印命令 */
        _do_completion_cmd(sh_hdl, &cp_info, &cmd_info, true, flag_parent);

        /* 恢复当前命令行显示 */
        _do_restore_line(sh_hdl);

    } while (0);

    return sh_hdl->cp_operate;
}

/**
 * @brief 根据输入的参数，结合当前命令行的状态，在已注册命令中查找并自执行一次自动补全命令/打印备选命令的全过程
 *
 * @param argc 命令数量
 * @param argv 命令字符串指针
 * @return sh_cp_op_t 自动补全命令/打印备选命令的执行结果
 */
sh_cp_op_t sh_completion_cmd(sh_t *sh_hdl, int argc, const char *argv[])
{
    SYS_ASSERT(sh_hdl != NULL, "");

    return _sh_completion_cmd(sh_hdl, argc, argv, 0);
}

/**
 * @brief 仅在 cp_fn() 中可用，根据当前命令行内容，在给出的参数结构中查找并自动补全参数/打印备选参数的全过程
 *
 * @param param 允许为 NULL。包含所有备选项的参数的结构。注意结构体必须有值为 NULL 的成员为结束。
 * @return sh_cp_op_t 自动补全参数/打印备选参数的执行结果
 */
sh_cp_op_t sh_completion_param(sh_t *sh_hdl, const sh_cp_param_t *param)
{
    SYS_ASSERT(sh_hdl != NULL, "");

    if (sh_hdl->exec_flag == 0)
    {
        sh_hdl->cp_operate = SH_CP_OP_NA;
    }

    do
    {
        sh_hdl->cp_resource_flag = 0;

        if (sh_hdl->cp_info == NULL)
        {
            if (sh_hdl->disable_echo == 0)
                SYS_LOG_WRN("This function is only used for functions with automatic parameter completion. @ref SH_SETUP_CMD\r\n");
            break;
        }

        if (param == 0)
        {
            break;
        }

        if (_do_completion_param(sh_hdl, sh_hdl->cp_info, param, false))
        {
            break; // 因为命令行被自动更新，不打印可用命令
        }
        if (sh_hdl->cp_info->match_num == 0)
        {
            break; // 没有匹配的命令
        }
        if (sh_hdl->tab_press_count == 0)
        {
            sh_hdl->tab_press_count = 1;
            break; // 暂停一次操作
        }

        /* 打印参数 */
        _do_completion_param(sh_hdl, sh_hdl->cp_info, param, true);

        /* 恢复当前命令行显示 */
        _do_restore_line(sh_hdl);

    } while (0);

    return sh_hdl->cp_operate;
}

/**
 * @brief 仅在 cp_fn() 中可用，根据当前命令行内容，逐个给出可供查找的提示符。为内部提供 自动补全/打印备选项 的具体内容。
 * 自动补全/打印备行项 将在 cp_fn() 退出后自动执行。
 * @ref @c fatfs_shell.c@_scan_files()/@_cmd_df_cp()
 *
 * @param arg_str 指定已输入的待自动补全的字符串，当为NULL时表示按当前命令行自动填充
 * @param res_str 候选字符串
 * @param help    帮助描述信息。如果值为 NULL 则不会被打印
 */
void sh_completion_resource(sh_t *sh_hdl, const char *arg_str, const char *res_str, const char *help)
{
    SYS_ASSERT(sh_hdl != NULL, "");

    if (sh_hdl->cp_info == NULL)
    {
        if (sh_hdl->cp_resource_flag == 0)
        {
            sh_hdl->cp_resource_flag = 1;
            if (sh_hdl->disable_echo == 0)
                SYS_LOG_WRN("This function is only used for functions with automatic parameter completion. @ref SH_SETUP_CMD\r\n");
        }
        return;
    }

    sh_hdl->cp_resource_flag = 1; // 标记执行了本函数

    if (res_str == NULL || *res_str == '\0')
    {
        return;
    }

    if (arg_str)
    {
        sh_hdl->cp_info->arg_str = arg_str;
        sh_hdl->cp_info->arg_len = strlen(arg_str);
    }

    sh_cmd_t sub_cmd = {
        .cmd = res_str,
        .help = help,
        .cmd_func = (__typeof(sub_cmd.cmd_func))1,
        .sub_fn = {.sub_cmd = NULL},
    };
    int max_lines = 0;
    if (help != NULL && *help != '\0')
    {
        max_lines = ~(1u << (sizeof(max_lines) * 8 - 1));
    }
    _completion_update(sh_hdl, sh_hdl->cp_info, &sub_cmd, (bool)!!sh_hdl->tab_press_count, max_lines, 0);
}

/**
 * @brief 获取当前输入的最后一个参数是否完整（以空格分开）
 *
 * @return true 已完整输入一个参数（结尾有空格）
 * @return false 正在输入参数
 */
static bool _arg_is_end(sh_t *sh_hdl)
{
    return (bool)(sh_ctrl_get_line(sh_hdl)[sh_hdl->cmd_input - 1] == ' ');
}

/**
 * @brief 同步热键功能：根据当前命令行缓存的指针位置，更新光标位置记录
 *
 */
static void _update_cursor(sh_t *sh_hdl)
{
    sh_hdl->tab_press_count = 0;
    sh_hdl->cp_resource_flag = 0;
    if (sh_hdl->port.set_cursor_hoffset)
    {
        if (sh_hdl->cmd_input != sh_hdl->sync_cursor)
        {
            int offset = strlen(sh_hdl->prompt) + (sh_hdl->cmd_buf - sh_hdl->cmd_line); // 输入命令的起始绝对位置
            int last_pos = sh_hdl->sync_cursor + offset;
            int new_pos = sh_hdl->cmd_input + offset;
            sh_hdl->port.set_cursor_hoffset(sh_hdl, last_pos, new_pos);
        }
    }
    sh_hdl->sync_cursor = sh_hdl->cmd_input;
}

/**
 * @brief 配合 sh_completion_resource() 使用，在下次进入 cp_fn() 时获取前一次自动补全的执行效果
 *
 * @return sh_cp_op_t 最后一次自动补全的执行效果
 */
sh_cp_op_t sh_get_cp_result(sh_t *sh_hdl)
{
    SYS_ASSERT(sh_hdl != NULL, "");

    return sh_hdl->cp_operate;
}

/**
 * @brief 同步热键功能：自动实例/打印命令
 *
 */
void sh_ctrl_tab(sh_t *sh_hdl)
{
    int argc;
    const char *argv[CONFIG_SH_MAX_PARAM];
    char cmd_param[sizeof(sh_hdl->cmd_line)];

    /* 复制当前显示的命令到 cmd_param ，并在光标处截断 */
    strcpy(cmd_param, sh_ctrl_get_line(sh_hdl));
    cmd_param[sh_hdl->cmd_input] = '\0';

    /* 解析参数并输出 argc, argv, cmd_param */
    _read_param(&argc, argv, cmd_param, cmd_param);

    /* 自动补全命令 */
    sh_completion_cmd(sh_hdl, argc, argv);
}

/**
 * @brief 同步热键功能：执行已缓存到的命令
 *
 * @return true 命令行中的命令被找到并且执行完毕
 * @return false 没有执行任何命令
 */
bool sh_ctrl_enter(sh_t *sh_hdl)
{
    int argc;
    const char *argv[CONFIG_SH_MAX_PARAM];
    char cmd_param[sizeof(sh_hdl->cmd_line)];
    bool ret = false;
    const char *cmd_line = sh_ctrl_get_line(sh_hdl);

    ++sh_hdl->exec_flag;

    /* 解析参数并输出 argc, argv, cmd_param */
    _read_param(&argc, argv, cmd_param, cmd_line);
    if (argc)
    {
        /* 将当前回翻的记录复制到缓存中 */
        _apply_history(sh_hdl);

        /* 根据 argc, argv，解析出 info */
        sh_cmd_info_t info = sh_cmd_info(sh_hdl, argc, argv);
        if (info.err_status == _SH_CMD_STATUS_Success)
        {
            _clear_argv(argc, argv);
            sh_hdl->cmd_return = info.match_cmd->cmd_func(sh_hdl, argc - info.match_count, &argv[info.match_count]);
            ret = true;
        }
        else
        {
            for (int i = 0; i < argc; i++)
            {
                sh_echo(sh_hdl, "%s%s", argv[i], i + 1 < argc ? " " : "");
            }
            if (info.err_status == _SH_CMD_STATUS_Incomplete)
            {
                sh_echo(sh_hdl, ": command not complete\r\n");
                sh_hdl->cmd_return = 2;
            }
            else
            {
                sh_echo(sh_hdl, ": command not found\r\n");
                sh_hdl->cmd_return = 1;
            }
            ret = false;
        }

        /* 去除重复的空格，但保持引号内的空格 */
        int index_src = 0;
        int index_dst = 0;
        char split_char = ' ';
        while (cmd_line[index_src] != '\0' && cmd_line[index_src] == ' ')
        {
            index_src++;
        }
        while (cmd_line[index_src] != '\0')
        {
            char c = cmd_line[index_src++];
            if (split_char == ' ')
            {
                if (c == '\'' || c == '"')
                {
                    split_char = c;
                }
            }
            else if (c == split_char)
            {
                split_char = ' ';
            }

            if (split_char == ' ' && c == ' ')
            {
                while (cmd_line[index_src] != '\0' && cmd_line[index_src] == ' ')
                {
                    index_src++;
                }

                if (cmd_line[index_src] == '\0')
                {
                    break;
                }
            }
            cmd_param[index_dst++] = c;
        }
        cmd_param[index_dst] = '\0';

        /* 尝试保存到历史记录中 */
        _store_history(sh_hdl, cmd_param);
    }
    else
    {
        sh_hdl->tab_press_count = 0;
        sh_hdl->cp_resource_flag = 0;
    }

    sh_reset_line(sh_hdl);

    --sh_hdl->exec_flag;

    return ret;
}

/**
 * @brief 同步热键功能：删除光标的后 n 个字符
 *
 * @param n 字符数
 * @return true 执行了动作
 * @return false 未执行动作
 */
bool sh_ctrl_delete(sh_t *sh_hdl, int n)
{
    if (n == 0)
    {
        return true;
    }

    _apply_history(sh_hdl);

    if (sh_hdl->cmd_input + n <= sh_hdl->cmd_stored)
    {
        for (int i = sh_hdl->cmd_input; i + n < sh_hdl->cmd_stored; i++) /* 使光标之后的数据往前移动一个字节 */
            sh_hdl->cmd_buf[i] = sh_hdl->cmd_buf[i + n];

        sh_hdl->cmd_stored -= n;
        sh_hdl->cmd_buf[sh_hdl->cmd_stored] = '\0';
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief 同步热键功能：删除光标的前 n 个字符
 *
 * @param n 字符数
 * @return true 执行了动作
 * @return false 未执行动作
 */
bool sh_ctrl_backspace(sh_t *sh_hdl, int n)
{
    if (n == 0)
    {
        return true;
    }

    _apply_history(sh_hdl);

    if (sh_hdl->cmd_input >= n)
    {
        sh_hdl->cmd_input -= n;
        sh_hdl->sync_cursor -= n;
        return sh_ctrl_delete(sh_hdl, n);
    }
    else
    {
        return false;
    }
}

/**
 * @brief 同步热键功能：查看上一个命令，并更新光标位置记录
 *
 * @return true 执行了动作
 * @return false 未执行动作
 */
bool sh_ctrl_up(sh_t *sh_hdl)
{
    if (sh_hdl->history_show < sh_hdl->history_valid_num)
    {
        ++sh_hdl->history_show;
        sh_ctrl_end(sh_hdl);
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief 同步热键功能：查看下一个命令，并更新光标位置记录
 *
 * @return true 执行了动作
 * @return false 未执行动作
 */
bool sh_ctrl_down(sh_t *sh_hdl)
{
    if (sh_hdl->history_show > 0)
    {
        --sh_hdl->history_show;
        sh_ctrl_end(sh_hdl);
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief 同步热键功能：光标左移，并更新光标位置记录
 *
 */
void sh_ctrl_left(sh_t *sh_hdl)
{
    if (sh_hdl->cmd_input > 0)
        --sh_hdl->cmd_input;

    _update_cursor(sh_hdl);
}

/**
 * @brief 同步热键功能：光标右移，并更新光标位置记录
 *
 */
void sh_ctrl_right(sh_t *sh_hdl)
{
    if (sh_hdl->cmd_input < _get_end_pos(sh_hdl))
        ++sh_hdl->cmd_input;

    _update_cursor(sh_hdl);
}

/**
 * @brief 同步热键功能：光标移到开头位置，并更新光标位置记录
 *
 */
void sh_ctrl_home(sh_t *sh_hdl)
{
    sh_hdl->cmd_input = 0;

    _update_cursor(sh_hdl);
}

/**
 * @brief 同步热键功能：光标移到结尾位置，并更新光标位置记录
 *
 */
void sh_ctrl_end(sh_t *sh_hdl)
{
    sh_hdl->cmd_input = _get_end_pos(sh_hdl);

    _update_cursor(sh_hdl);
}

/**
 * @brief 同步热键功能：光标移到指定位置（不包括提示符，起始位置为0），并更新光标位置记录
 *
 * @param pos 0.._get_end_pos(sh_hdl)
 */
void sh_ctrl_set_cursor(sh_t *sh_hdl, int pos)
{
    if (pos < 0)
    {
        sh_hdl->cmd_input = 0;
    }
    else if (pos > _get_end_pos(sh_hdl))
    {
        sh_hdl->cmd_input = _get_end_pos(sh_hdl);
    }
    else
    {
        sh_hdl->cmd_input = pos;
    }
    _update_cursor(sh_hdl);
}

/**
 * @brief 同步热键功能：光标移到当前单词的开头，并更新光标位置记录
 *
 */
void sh_ctrl_word_head(sh_t *sh_hdl)
{
    const char *line = sh_ctrl_get_line(sh_hdl);
    int pos = sh_hdl->cmd_input;
    while (pos)
    {
        if (_IS_ALPHA(line[pos - 1]) || _IS_DIGIT(line[pos - 1]))
        {
            break;
        }
        --pos;
    }

    while (pos)
    {
        if (!_IS_ALPHA(line[pos - 1]) && !_IS_DIGIT(line[pos - 1]))
        {
            break;
        }
        --pos;
    }
    sh_ctrl_set_cursor(sh_hdl, pos);
}

/**
 * @brief 同步热键功能：光标移到当前单词的结尾，并更新光标位置记录
 *
 */
void sh_ctrl_word_tail(sh_t *sh_hdl)
{
    const char *line = sh_ctrl_get_line(sh_hdl);
    int pos = sh_hdl->cmd_input;
    while (line[pos] != '\0')
    {
        if (_IS_ALPHA(line[pos]) || _IS_DIGIT(line[pos]))
        {
            break;
        }
        ++pos;
    }

    while (line[pos] != '\0')
    {
        if (!_IS_ALPHA(line[pos]) && !_IS_DIGIT(line[pos]))
        {
            break;
        }
        ++pos;
    }
    sh_ctrl_set_cursor(sh_hdl, pos);
}

/**
 * @brief 同步热键功能：打印一次命令行内容
 *
 */
void sh_ctrl_print_cmd_line(sh_t *sh_hdl)
{
    sh_echo(sh_hdl, _COLOR_G);
    sh_echo(sh_hdl, sh_ctrl_get_prompt(sh_hdl));
    if (sh_hdl->select_cmd)
    {
        sh_echo(sh_hdl, _COLOR_P);
        sh_echo(sh_hdl, "%s: ", sh_hdl->cmd_line);
    }
    sh_echo(sh_hdl, _COLOR_END);
    const char *line = sh_ctrl_get_line(sh_hdl);
    sh_echo(sh_hdl, line);

    sh_hdl->cmd_input = strlen(line);
    sh_hdl->sync_cursor = sh_hdl->cmd_input;
    _update_cursor(sh_hdl);
}

/**
 * @brief 同步热键功能：擦除光标前的单词内容
 *
 * @return unsigned 成功擦除的字符数
 */
unsigned sh_ctrl_del_word(sh_t *sh_hdl)
{
    const char *line = sh_ctrl_get_line(sh_hdl);
    int pos = sh_hdl->cmd_input;
    int n = 0;
    while (pos)
    {
        if (_IS_ALPHA(line[pos - 1]) || _IS_DIGIT(line[pos - 1]))
        {
            break;
        }
        --pos;
        ++n;
    }

    while (pos)
    {
        if (!_IS_ALPHA(line[pos - 1]) && !_IS_DIGIT(line[pos - 1]))
        {
            break;
        }
        --pos;
        ++n;
    }

    _store_cmd_bak(sh_hdl, line, pos, n);

    sh_ctrl_backspace(sh_hdl, n);

    return n;
}

/**
 * @brief 同步热键功能：擦除光标前到行首的全部内容
 *
 * @return unsigned 成功擦除的字符数
 */
unsigned sh_ctrl_del_left(sh_t *sh_hdl)
{
    const char *line = sh_ctrl_get_line(sh_hdl);
    int pos = sh_hdl->cmd_input;

    _store_cmd_bak(sh_hdl, line, 0, pos);

    sh_ctrl_backspace(sh_hdl, pos);
    return pos;
}

/**
 * @brief 同步热键功能：擦除光标后到行尾的全部内容
 *
 * @return unsigned 成功擦除的字符数
 */
unsigned sh_ctrl_del_right(sh_t *sh_hdl)
{
    const char *line = sh_ctrl_get_line(sh_hdl);
    int pos = sh_hdl->cmd_input;
    int n = strlen(line) - pos;

    _store_cmd_bak(sh_hdl, line, pos, n);

    sh_ctrl_delete(sh_hdl, n);

    return n;
}

/**
 * @brief 同步热键功能：插入 sh_ctrl_del_word(), sh_ctrl_del_left(), sh_ctrl_del_right() 擦除的内容
 *
 * @return true 执行了动作
 * @return false 未执行动作
 */
bool sh_ctrl_undelete(sh_t *sh_hdl)
{
    if (sh_hdl->cmd_bank && sh_hdl->bank_size)
    {
        if (sh_hdl->cmd_bank[0])
        {
            _insert_str(sh_hdl, sh_hdl->cmd_bank);
            return true;
        }
    }
    return false;
}

/**
 * @brief 同步热键功能：查询当前是否使用了 select 命令进入的模块模式（快捷命令模式）
 *
 * @return true 当前在模块模式
 * @return false 当前未在模块模式
 */
bool sh_ctrl_is_module(sh_t *sh_hdl)
{
    return (bool)(!!sh_hdl->select_cmd);
}

/**
 * @brief 设置当前指针（光标）位置（不包括提示符和模块提示）
 *
 * @param input_pos 0.._get_end_pos(sh_hdl)
 */
void sh_ctrl_set_input_pos(sh_t *sh_hdl, uint16_t input_pos)
{
    if (input_pos > _get_end_pos(sh_hdl))
        input_pos = _get_end_pos(sh_hdl);

    sh_hdl->cmd_input = input_pos;

    _update_cursor(sh_hdl);
}

/**
 * @brief 获取当前指针（光标）位置（不包括提示符和模块提示），起始位置为0
 *
 * @param input_pos 0.._get_end_pos(sh_hdl)
 */
uint16_t sh_ctrl_get_input_pos(sh_t *sh_hdl)
{
    return sh_hdl->cmd_input;
}

/**
 * @brief 根据当前显示内容（包含回翻的历史记录），获取当前光标位置（提示符+模块提示+命令）
 *
 * @return uint16_t 获取当前光标位置（提示符+模块提示+命令）
 */
uint16_t sh_ctrl_get_line_pos(sh_t *sh_hdl)
{
    return strlen(sh_hdl->prompt) +
           (sh_hdl->cmd_buf - sh_hdl->cmd_line) +
           sh_hdl->cmd_input;
}

/**
 * @brief 根据当前显示内容（包含回翻的历史记录），获取最大光标位置（包含提示符和模块提示）
 * 用于模拟清除当前行
 *
 * @return uint16_t 当前命令行显示的总字符数
 */
uint16_t sh_ctrl_get_line_len(sh_t *sh_hdl)
{
    return strlen(sh_hdl->prompt) +
           (sh_hdl->cmd_buf - sh_hdl->cmd_line) +
           _get_end_pos(sh_hdl);
}

/**
 * @brief 获取命令行部分的字符（提示符+模块提示+命令）
 *
 * @param pos 命令行位置 0.._get_end_pos(sh_hdl)
 * @return char 字符
 */
char sh_ctrl_get_line_char(sh_t *sh_hdl, uint16_t pos)
{
    int test_len;

    test_len = strlen(sh_hdl->prompt);
    if (pos < test_len)
    {
        return sh_hdl->prompt[pos];
    }
    pos -= test_len;

    test_len = sh_hdl->cmd_buf - sh_hdl->cmd_line;
    if (pos < test_len)
    {
        return sh_hdl->cmd_line[pos];
    }
    pos -= test_len;

    test_len = _get_end_pos(sh_hdl);
    if (pos < test_len)
    {
        if (sh_hdl->history_show)
            return sh_hdl->cmd_history[sh_hdl->history_index[sh_hdl->history_show - 1] + pos];
        else
            return sh_hdl->cmd_buf[pos];
    }

    return '\0';
}

/* 内部默认命令 -------------------------------------------------------------------------------------------- */

#if defined(MIX_SHELL)

SH_CMD_FN(_cmd_print_init_fn)
{
    extern sys_init_t __sys_init_leader_0;
    extern sys_init_t __sys_init_leader_e;
    sys_init_t *start = &__sys_init_leader_0;
    sys_init_t *end = &__sys_init_leader_e;
    int match_count = 0;
    sh_echo(sh_hdl, "Automatic initialization list >>>\r\n");
    while (start < end)
    {
        if (start->level + start->prior != 0 &&
            start->level * 100 + start->prior != 100 &&
            start->level * 100 + start->prior != 999)
        {
            const char *lev2char[] = {
                "BOARD",
                "PREV",
                "DEVICE",
                "COMPONENT",
                "ENV",
                "APP",
            };
            char buf[0x200];
            SYS_SNPRINT(buf, sizeof(buf), "%-10s %d-%-2d  %s:%d  @%s",
                        start->level < sizeof(lev2char) / sizeof(lev2char[0]) ? lev2char[start->level] : "MISC",
                        start->level,
                        start->prior,
                        _FILENAME(start->file),
                        start->line,
                        start->fn_name);

            int result = !argc;
            for (int i = 0; i < argc; i++)
            {
                if (strstr(buf, argv[i]))
                {
                    result = 1;
                    break;
                }
            }

            if (result)
            {
                sh_echo(sh_hdl, "%s\r\n", buf);
                match_count++;
            }
        }
        start = &start[1];
    }
    sh_echo(sh_hdl, "Total: %d\r\n", match_count);
    return 0;
}

SH_CMD_FN(_cmd_print_module_struct)
{
    extern const module_t __sys_module_leader_0;
    extern const module_t __sys_module_leader_9;
    const module_t *start = &__sys_module_leader_0;
    const module_t *end = &__sys_module_leader_9;
    int match_count = 0;
    sh_echo(sh_hdl, "Internal module cmd list >>>\r\n");

    const struct
    {
        module_type_t type;
        const char *name;
    } type2char[] = {
        {MODULE_TYPE_DEVICE, "DEVICE"},
    };

    for (int n = 0; n < sizeof(type2char) / sizeof(type2char[0]); n++)
    {
        while (start < end)
        {
            if (start->type == type2char[n].type)
            {
                char buf[0x200];
                SYS_SNPRINT(buf, sizeof(buf), "%p    %-10s %-10s %s:%d",
                            start->obj,
                            type2char[n].name,
                            start->name,
                            _FILENAME(start->file),
                            start->line);

                int result = !argc;
                for (int i = 0; i < argc; i++)
                {
                    if (strstr(buf, argv[i]))
                    {
                        result = 1;
                        break;
                    }
                }

                if (result)
                {
                    sh_echo(sh_hdl, "%s\r\n", buf);
                    match_count++;
                }
            }
            start = &start[1];
        }
    }

    sh_echo(sh_hdl, "Total: %d\r\n", match_count);
    return 0;
}

#endif /* #if defined(MIX_SHELL) */

SH_CMD_FN(_cmd_history)
{
    if (argc == 0)
    {
        for (int i = sh_hdl->history_valid_num; i--;)
        {
            sh_echo(sh_hdl, "%s\r\n", &sh_hdl->cmd_history[sh_hdl->history_index[i]]);
        }
    }
    else
    {
        if (strcmp(argv[0], "enable") == 0)
        {
            sh_hdl->disable_history = false;
        }
        else if (strcmp(argv[0], "disable") == 0)
        {
            sh_hdl->disable_history = true;
        }
        else
        {
            sh_echo(sh_hdl, "%s: parameter error\r\n", argv[0]);
        }
    }
    return 0;
}

SH_CMD_FN(_cmd_select)
{
    if (argc)
    {
        sh_cmd_info_t cmd_info = sh_cmd_info(sh_hdl, argc, argv);
        if (cmd_info.err_status == _SH_CMD_STATUS_Incomplete)
        {
            sh_hdl->select_reg_struct = cmd_info.reg_struct;
            sh_hdl->select_cmd = cmd_info.match_cmd;

            /* 把 cmd_param 中分散的参数以空格分隔，合并成一字符串并保存 */
            char *p = sh_hdl->cmd_line;
            for (int i = 0; i < argc; i++)
            {
                for (int n = 0; argv[i][n] != '\0'; n++)
                {
                    *p++ = argv[i][n];
                }
                if (i + 1 < argc)
                {
                    *p++ = ' ';
                }
                else
                {
                    *p++ = '\0';
                }
            }
            sh_hdl->cmd_buf = p;
            return 0;
        }
        else
        {
            sh_echo(sh_hdl, "%s: not a parent command\r\n", argv[0]);
        }
    }
    else
    {
        if (sh_hdl->select_cmd)
        {
            sh_hdl->select_cmd = NULL;
            sh_hdl->cmd_buf = sh_hdl->cmd_line;
            return 0;
        }
    }

    return -1;
}

SH_CMD_CP_FN(_cmd_select_param)
{
    _sh_completion_cmd(sh_hdl, argc, argv, 1);
}

SH_CMD_CP_FN(_cmd_history_param)
{
    sh_completion_resource(sh_hdl, NULL, "enable", NULL);
    sh_completion_resource(sh_hdl, NULL, "disable", NULL);
}

SH_CMD_FN(_cmd_version)
{
    sh_echo(sh_hdl, "sh v" SH_VERSION " build %04d-%02d-%02d\r\n", _YEAR, _MONTH, _DAY);
    return 0;
}

SH_CMD_FN(_cmd_exit)
{
    if (sh_hdl->select_cmd)
    {
        sh_hdl->select_cmd = NULL;
        sh_hdl->cmd_buf = sh_hdl->cmd_line;
        return 0;
    }
    else if (sh_hdl->port.disconnect)
    {
        sh_hdl->port.disconnect();
        return 0;
    }
    else
    {
        extern void drv_hal_sys_exit(void);
        drv_hal_sys_exit();
        return -1;
    }
}

SH_CMD_FN(_cmd_print_help)
{
    sh_cmd_info_t info = sh_cmd_info(sh_hdl, argc, argv);

    if (argc == 0)
    {
        sh_cp_info_t cp_info = _completion_init("");
        sh_echo(sh_hdl, "Command list >>>");
        cp_info.match_num = 1;
        _do_completion_cmd(sh_hdl, &cp_info, NULL, false, 0);
        _do_completion_cmd(sh_hdl, &cp_info, NULL, true, 0);
        return 0;
    }

    if (info.err_status == _SH_CMD_STATUS_Bad)
    {
        sh_echo(sh_hdl, "sh: help: not found Command '");
        for (int i = 0; i < argc; i++)
        {
            sh_echo(sh_hdl, "%s%s", argv[i], i + 1 < argc ? " " : "'\r\n");
        }
        return 0;
    }

    int total_len = 0;
    for (int i = 0; i < info.match_count; i++)
    {
        total_len += strlen(argv[i]) + 1;
    }
    total_len += 3;
    for (int i = 1; i <= info.match_count; i++)
    {
        sh_cmd_info_t tmp = sh_cmd_info(sh_hdl, i, argv);

        if (i == 1)
        {
            sh_echo(sh_hdl, "build in %s:%d\r\n", _FILENAME(info.reg_struct->file), info.reg_struct->line);
        }

        int print_len = 0;
        for (int j = 0; j < i; j++)
        {
            print_len += sh_echo(sh_hdl, "%s%s", argv[j], j + 1 < i ? " " : "");
        }
        for (int j = 0; j < total_len - print_len; j++)
        {
            sh_echo(sh_hdl, " ");
        }
        sh_echo(sh_hdl, "-- %s\r\n", tmp.match_cmd->help);

        if (i == info.match_count)
        {
            if (info.err_status == _SH_CMD_STATUS_Success && info.match_cmd->sub_fn.cp_fn == NULL)
            {
                break;
            }
            for (int j = 0; j < i; j++)
            {
                sh_echo(sh_hdl, "%s%s", argv[j], j + 1 < i ? " " : "");
            }
            if (info.err_status == _SH_CMD_STATUS_Incomplete)
            {
                sh_echo(sh_hdl, " <...>\r\n"); // 命令未完整
            }
            else if (info.match_cmd->sub_fn.cp_fn)
            {
                sh_echo(sh_hdl, " [...]\r\n"); // 带参数自动补全的命令
            }
        }
    }

    return 0;
}

SH_CMD_CP_FN(_cmd_print_help_param)
{
    sh_completion_cmd(sh_hdl, argc, argv);
}

SH_CMD_FN(_cmd_echo_on)
{
    sh_hdl->disable_echo = 0;
    return 0;
}

SH_CMD_FN(_cmd_echo_off)
{
    sh_hdl->disable_echo = 1;
    return 0;
}

SH_CMD_FN(_cmd_parse_test)
{
    for (int i = 0; i < argc; i++)
    {
        sh_parse_t pv = sh_parse_value(argv[i]);
        switch (pv.type)
        {
        case _PARSE_TYPE_STRING: // 字符串
            sh_echo(sh_hdl, "param%d: type = string \"%s\"\r\n",
                    i + 1,
                    pv.value.val_string);
            break;
        case _PARSE_TYPE_INTEGER: // 带符号整型
            sh_echo(sh_hdl, "param%d: type = inteter, value = %u(dec) 0x%x(hex) 0%o(oct)\r\n",
                    i + 1,
                    pv.value.val_unsigned, pv.value.val_unsigned, pv.value.val_unsigned);
            break;
        case _PARSE_TYPE_UNSIGNED: // 无符号整型
            sh_echo(sh_hdl, "param%d: type = inteter, value = %d(dec) 0x%x(hex) 0%o(oct)\r\n",
                    i + 1,
                    pv.value.val_integer, pv.value.val_integer, pv.value.val_integer);
            break;
        case _PARSE_TYPE_FLOAT: // 浮点
            sh_echo(sh_hdl, "param%d: type = float,   value = %f\r\n",
                    i + 1,
                    pv.value.val_float);
            break;

        default:
            break;
        }
    }
    return 0;
}

SH_DEF_SUB_CMD(
    _cmd_echo_sublist,
    SH_SETUP_CMD("on", "Enable to feedback the command line", _cmd_echo_on, NULL),    //
    SH_SETUP_CMD("off", "Disable to feedback the command line", _cmd_echo_off, NULL), //
);

SH_DEF_SUB_CMD(
    _cmd_sh_sublist,
    SH_SETUP_CMD("echo", "Turn on/off echo through sh_echo()", NULL, _cmd_echo_sublist), //
    SH_SETUP_CMD("history", "Show history control", _cmd_history, _cmd_history_param),   //
#if defined(MIX_SHELL)
    SH_SETUP_CMD("list-init", "List all auto initialize function\r\n\t* Usage: list-init [filter]", _cmd_print_init_fn, NULL),   //
    SH_SETUP_CMD("list-module", "List all module structure\r\n\t* Usage: list-module [filter]", _cmd_print_module_struct, NULL), //
#endif
    SH_SETUP_CMD("version", "Print the shell version", _cmd_version, NULL),                 //
    SH_SETUP_CMD("parse-value", "Parse the value of a string demo", _cmd_parse_test, NULL), //
);

SH_REGISTER_CMD(
    register_internal_command,
    SH_SETUP_CMD("sh", "Internal command", NULL, _cmd_sh_sublist),                                   //
    SH_SETUP_CMD("help", "Print the complete root command", _cmd_print_help, _cmd_print_help_param), //
    SH_SETUP_CMD("select", "Select parent command", _cmd_select, _cmd_select_param),                 //
    SH_SETUP_CMD("exit", "Exit parent command or disconnect", _cmd_exit, NULL),                      //
);

/**
 * @brief 用于显式初始化内部函数。
 * 当自动初始化函数不适用时，使用此函数可强制初始化内部的一些功能。包含：
 * 1. 执行 register_internal_command() 注册内部的基本命令
 * 2. 执行 sh_init_vt100() 控制相关的初始化
 *
 * @param out 定义执行 vprintf 的函数函数
 */
void sh_register_external(sh_vprint_fn out)
{
    sh_register_cmd(&register_internal_command);

    if (out == NULL)
    {
        out = SYS_VPRINT;
    }
    if (sh_init_vt100(&g_uart_handle_vt100, out, NULL) == 0)
    {
        static uint8_t history_mem[255];
        static uint8_t bank_mem[CONFIG_SH_MAX_LINE_LEN];
        sh_config_history_mem(&g_uart_handle_vt100, history_mem, sizeof(history_mem));
        sh_config_backup_mem(&g_uart_handle_vt100, bank_mem, sizeof(bank_mem));
    }
}
