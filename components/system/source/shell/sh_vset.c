/**
 * @file sh_vset.h.c
 * @author LokLiang
 * @brief sh_vset.h 模块源码
 * @version 0.1
 * @date 2023-09-22
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "sh_vset.h"
#include <string.h>

#undef CONFIG_SYS_LOG_LEVEL
#define CONFIG_SYS_LOG_LEVEL SYS_LOG_LEVEL_INF
#define SYS_LOG_DOMAIN "VSET"
#define CONS_ABORT()
#include "sys_log.h"

#define __ARR_SIZE(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

#define _TYPE_FLAG_UNSIGNED (1 << 0)
#define _TYPE_FLAG_INTEGER (1 << 1)
#define _TYPE_FLAG_FLOAT (1 << 2)
#define _TYPE_FLAG_STRING (1 << 3)

#define _PARAM_CB()                      \
    do                                   \
    {                                    \
        if (param->cb)                   \
            param->cb(s_sh_hdl, &value); \
    } while (0)

static sh_t *s_sh_hdl;
static vset_global_cb s_global_cb;

static bool _is_in_option(const char *input, const sh_vset_param_t *p, unsigned size);
static const sh_vset_param_t *_find_option(const char *input, const sh_vset_param_t *p, unsigned size);
static bool _do_completing_option(sh_t *sh_hdl, int argc, const char *argv[], const sh_vset_param_t *p, unsigned size);
static bool _do_completing_enum(const char *option, const sh_vset_param_t *p, unsigned size);

static int _set_dest_unsigned(const __vset_param_t *param, unsigned int value, unsigned int low, unsigned int high);
static int _set_dest_integer(const __vset_param_t *param, signed int value, signed int low, signed int high);
static int _set_dest_float(const __vset_param_t *param, float value, float low, float high);
static int _set_dest_string(const __vset_param_t *param, const char *str, unsigned bufsize);
static int _set_dest_enum(const __vset_param_t *param, const char *value, int type_flag);

static unsigned int _get_unsigned(const __vset_param_t *param);
static signed int _get_integer(const __vset_param_t *param);
static float _get_float(const __vset_param_t *param);
static char *_get_string(const __vset_param_t *param);
static int _enum_format(char *enum_buf, int buf_len, const char *enum_str);
static int _get_enum_by_key(const char *key, char **match_value, char *enum_buf);
static int _get_enum_by_param(const __vset_param_t *param, char **match_key, char **match_value, char *enum_buf, bool print_list, bool print_cp);

/**
 * @brief s_attr_to_type_tbl
 * 由变量的具体类型转换到可用于参数解析的范围
 */
static int const s_attr_to_type_tbl[] = {
    [__TYPE_ATTR_CHR] = _PARSE_TYPE_INTEGER,
    [__TYPE_ATTR_BOOL] = _PARSE_TYPE_INTEGER,
    [__TYPE_ATTR_U8] = _PARSE_TYPE_UNSIGNED,
    [__TYPE_ATTR_S8] = _PARSE_TYPE_INTEGER,
    [__TYPE_ATTR_U16] = _PARSE_TYPE_UNSIGNED,
    [__TYPE_ATTR_S16] = _PARSE_TYPE_INTEGER,
    [__TYPE_ATTR_U32] = _PARSE_TYPE_UNSIGNED,
    [__TYPE_ATTR_S32] = _PARSE_TYPE_INTEGER,
    [__TYPE_ATTR_U64] = _PARSE_TYPE_UNSIGNED,
    [__TYPE_ATTR_S64] = _PARSE_TYPE_INTEGER,
    [__TYPE_ATTR_FLOAT] = _PARSE_TYPE_FLOAT,
    [__TYPE_ATTR_DOUBLE] = _PARSE_TYPE_FLOAT,
    [__TYPE_ATTR_STR] = _PARSE_TYPE_STRING,
};

/**
 * @brief 初始化设置
 *
 * @param sh_hdl    设置 shell 的句柄
 * @param cb        当任意一个值被修改时回调此函数，可设为 NULL
 */
void vset_init(sh_t *sh_hdl, vset_global_cb cb)
{
    s_sh_hdl = sh_hdl;
    s_global_cb = cb;
}

/**
 * @brief 直接执行一次 vset_init() 所定义的回调函数
 *
 */
void vset_force_cb(void)
{
    vset_global_cb cb = s_global_cb;
    if (cb)
    {
        cb(s_sh_hdl);
    }
}

/**
 * @brief 一次性根据选项结构 p 解析 argv, 自动选择对应的设置函数进行对变量的设置。
 * 这将导致输入的参数 argc 和 argv 被更新。
 *
 * @details
 * 这将一将性解析全部符合 p 内部符合的参数，并执行对应的处理函数，成功设置的 argc 和 argv 会被删除。
 * 注意如果有任何处理失败的函数不会立即返回，已成功设置的值也不会被恢复，直到全部 argv 被解析过一次后返回。
 *
 * @param sh_hdl    原参数
 * @param argc[i/o] 指向原参数，可能被更新
 * @param argv[i/o] 原参数指针值，可能被更新
 * @param p         选项描述结构体
 * @param size      选项描述结构体的长度
 * @retval 0 所有存在于 sh_vset_param_t::option 的选项被正确设置
 * @retval != 0 首个设置错误的 sh_vset_param_t::option
 */
int vset_option_set(sh_t *sh_hdl, int *argc, const char *argv[], const sh_vset_param_t *p, unsigned size)
{
    int argc_old = *argc;
    const char *err_option = NULL;

    /* 遍历输入参数 argv 在 p 中找到匹配的 p[i].option 对应的 p[i].set_func 并执行 */
    for (int i = 0; i < argc_old; i++)
    {
        for (int j = 0; j < size / sizeof(*p); j++)
        {
            const sh_vset_param_t *pset = &p[j];
            if (strcmp(argv[i], pset->option) == 0) // 匹配到选项
            {
                if (pset->set_func) // 该选有对应的设置函数
                {
                    bool flag = false;
                    argv[i] = NULL;

                    const char *str[] = {
                        "",
                        "",
                    };
                    if (i + 1 < argc_old)
                    {
                        ++i;
                        str[0] = argv[i];
                        argv[i] = NULL;

                        if (strcmp("?", str[0]) == 0)
                        {
                            flag = true;
                            sh_echo(s_sh_hdl, "选项:     %s\r\n", pset->option);
                        }
                    }

                    int ret = pset->set_func(str);
                    if (flag)
                    {
                        sh_echo(s_sh_hdl, "========\r\n");
                    }
                    if (ret != 0)
                    {
                        if (err_option == NULL)
                        {
                            err_option = pset->option;
                        }
                        sh_echo(s_sh_hdl, "( 选项 %s )\r\n", pset->option);
                        sh_echo(s_sh_hdl, "========\r\n");
                    }
                }

                break;
            }
        }
    }

    /* 更新已被处理的输入参数 argc 和 argv */
    int argc_new = 0;
    for (int i = 0; i < argc_old; i++)
    {
        if (argv[i] && *argv[i] != '\0')
        {
            argv[argc_new++] = argv[i]; // 更新 argv
        }
    }
    *argc = argc_new; // 更新 argc
    while (argc_new < argc_old)
    {
        argv[argc_new++] = NULL;
    }

    return (int)err_option;
}

/**
 * @brief 为 cp_fn 执行与 set_func 对应的参数补全动作
 *
 * @param sh_hdl    原参数
 * @param argc      原参数
 * @param argv      原参数指针值，可能被更新
 * @param flag      原参数
 * @param p         选项描述结构体
 * @param size      选项描述结构体的长度
 * @retval true 有补全动作， cp_fn 函数不需要进行任何动作
 * @retval false 未补全动作， cp_fn 函数可自行执行补全
 */
bool vset_option_cp(sh_t *sh_hdl, int argc, const char *argv[], bool flag, const sh_vset_param_t *p, unsigned size)
{
    if (flag == false) // 一个参数正在输入
    {
        if (argc > 0 && _is_in_option(argv[argc - 1], p, size)) // 前正在输入的参数有对应存在的候选选项
        {
            _do_completing_option(sh_hdl, argc, argv, p, size); // 执行对选项的补全
            return true;
        }
        else if (argc > 1)
        {
            return _do_completing_enum(argv[argc - 2], p, size); // 执行对 SET_ENUM 中的 ENUM_STR 的补全
        }
        else
        {
            return false;
        }
    }
    else // 当前参数未有任何输入
    {
        const sh_vset_param_t *pset;
        if (argc > 0 && (pset = _find_option(argv[argc - 1], p, size)) != NULL) // 找到上一个输入的参数的对应选项
        {
            vset_cp_enum(1, false, pset->enum_str);
            return true;
        }
        else
        {
            _do_completing_option(sh_hdl, argc, argv, p, size); // 执行对选项的补全
            return true;
        }
    }
}

int vset_unsigned(const __vset_param_t *param, const char *argv[], unsigned int low, unsigned int high)
{
    SYS_ASSERT(s_sh_hdl, "未使用 vset_init() 初始化");
    SYS_ASSERT(param->attr < __ARR_SIZE(s_attr_to_type_tbl), "param->attr 的值不在 __type_attr_t 所定义的范围");
    SYS_ASSERT(s_attr_to_type_tbl[param->attr] == _PARSE_TYPE_UNSIGNED, "待设置的参数类型不是正整数");

    if (s_sh_hdl == NULL)
    {
        return -1;
    }
    if (param->attr >= __ARR_SIZE(s_attr_to_type_tbl))
    {
        sh_echo(s_sh_hdl, "param->attr 的值不在 __type_attr_t 所定义的范围\r\n");
        return -1;
    }
    if (s_attr_to_type_tbl[param->attr] != _PARSE_TYPE_UNSIGNED)
    {
        sh_echo(s_sh_hdl, "待设置的参数类型不是正整数\r\n");
        return -1;
    }

    if (argv[0] == NULL || *argv[0] == '\0')
    {
        sh_echo(s_sh_hdl, "缺少参数\r\n");
        return -1;
    }

    if (strcmp(argv[0], "?") == 0)
    {
        if (high < low)
        {
            low ^= high;
            high ^= low;
            low ^= high;
        }
        sh_echo(s_sh_hdl, "有效范围: %d .. %d\r\n", low, high);
        sh_echo(s_sh_hdl, "当前值:   %u\r\n", _get_unsigned(param));
        return 0;
    }

    sh_parse_t pv = sh_parse_value(argv[0]);
    switch (pv.type)
    {
    case _PARSE_TYPE_UNSIGNED: // 解析出的参数格式是无符号整型
        return _set_dest_unsigned(param, pv.value.val_unsigned, low, high);

    default:
        sh_echo(s_sh_hdl, "参数 '%s' 格式错误, 请输入正整数\r\n", argv[0]);
        return -1;
    }
}

int vset_integer(const __vset_param_t *param, const char *argv[], signed int low, signed int high)
{
    SYS_ASSERT(s_sh_hdl, "未使用 vset_init() 初始化");
    SYS_ASSERT(param->attr < __ARR_SIZE(s_attr_to_type_tbl), "param->attr 的值不在 __type_attr_t 所定义的范围");
    SYS_ASSERT(s_attr_to_type_tbl[param->attr] == _PARSE_TYPE_INTEGER, "待设置的参数类型不是带符号整数");

    if (s_sh_hdl == NULL)
    {
        return -1;
    }
    if (param->attr >= __ARR_SIZE(s_attr_to_type_tbl))
    {
        sh_echo(s_sh_hdl, "param->attr 的值不在 __type_attr_t 所定义的范围\r\n");
        return -1;
    }
    if (s_attr_to_type_tbl[param->attr] != _PARSE_TYPE_INTEGER)
    {
        sh_echo(s_sh_hdl, "待设置的参数类型不是带符号整数\r\n");
        return -1;
    }

    if (argv[0] == NULL || *argv[0] == '\0')
    {
        sh_echo(s_sh_hdl, "缺少参数\r\n");
        return -1;
    }

    if (strcmp(argv[0], "?") == 0)
    {
        if (high < low)
        {
            low ^= high;
            high ^= low;
            low ^= high;
        }
        sh_echo(s_sh_hdl, "有效范围: %d .. %d\r\n", low, high);
        sh_echo(s_sh_hdl, "当前值:   %d\r\n", _get_integer(param));
        return 0;
    }

    sh_parse_t pv = sh_parse_value(argv[0]);
    switch (pv.type)
    {
    case _PARSE_TYPE_INTEGER: // 解析出的参数格式是带符号整型
        return _set_dest_integer(param, pv.value.val_integer, low, high);

    case _PARSE_TYPE_UNSIGNED: // 解析出的参数格式是无符号整型
        if (pv.value.val_unsigned > (1u << (sizeof(pv.value.val_unsigned) * 8 - 1)) - 1)
        {
            sh_echo(s_sh_hdl, "参数 %u 溢出\r\n", pv.value.val_unsigned);
            return -1;
        }
        return _set_dest_integer(param, pv.value.val_unsigned, low, high);

    default:
        sh_echo(s_sh_hdl, "参数 '%s' 格式错误, 请输入整数\r\n", argv[0]);
        return -1;
    }
}

int vset_float(const __vset_param_t *param, const char *argv[], float low, float high)
{
    SYS_ASSERT(s_sh_hdl, "未使用 vset_init() 初始化");
    SYS_ASSERT(param->attr < __ARR_SIZE(s_attr_to_type_tbl), "param->attr 的值不在 __type_attr_t 所定义的范围");
    SYS_ASSERT(s_attr_to_type_tbl[param->attr] == _PARSE_TYPE_FLOAT, "待设置的参数类型不是浮点数");

    if (s_sh_hdl == NULL)
    {
        return -1;
    }
    if (param->attr >= __ARR_SIZE(s_attr_to_type_tbl))
    {
        sh_echo(s_sh_hdl, "param->attr 的值不在 __type_attr_t 所定义的范围\r\n");
        return -1;
    }
    if (s_attr_to_type_tbl[param->attr] != _PARSE_TYPE_FLOAT)
    {
        sh_echo(s_sh_hdl, "待设置的参数类型不是浮点数\r\n");
        return -1;
    }

    if (argv[0] == NULL || *argv[0] == '\0')
    {
        sh_echo(s_sh_hdl, "缺少参数\r\n");
        return -1;
    }

    if (strcmp(argv[0], "?") == 0)
    {
        if (high < low)
        {
            float tmp = low;
            low = high;
            high = tmp;
        }
        sh_echo(s_sh_hdl, "有效范围: %f .. %f\r\n", low, high);
        sh_echo(s_sh_hdl, "当前值:   %f\r\n", _get_float(param));
        return 0;
    }

    sh_parse_t pv = sh_parse_value(argv[0]);
    switch (pv.type)
    {
    case _PARSE_TYPE_INTEGER: // 解析出的参数格式是带符号整型
        return _set_dest_float(param, pv.value.val_integer, low, high);

    case _PARSE_TYPE_UNSIGNED: // 解析出的参数格式是无符号整型
        return _set_dest_float(param, pv.value.val_unsigned, low, high);

    case _PARSE_TYPE_FLOAT: // 解析出的参数格式是浮点数
        return _set_dest_float(param, pv.value.val_float, low, high);

    default:
        sh_echo(s_sh_hdl, "参数 '%s' 格式错误, 请输入数值\r\n", argv[0]);
        return -1;
    }
}

int vset_str(const __vset_param_t *param, const char *argv[], unsigned bufsize)
{
    SYS_ASSERT(s_sh_hdl, "未使用 vset_init() 初始化");
    SYS_ASSERT(param->attr < __ARR_SIZE(s_attr_to_type_tbl), "param->attr 的值不在 __type_attr_t 所定义的范围");
    SYS_ASSERT(s_attr_to_type_tbl[param->attr] == _PARSE_TYPE_STRING, "待设置的参数类型不是字符串");

    if (s_sh_hdl == NULL)
    {
        return -1;
    }
    if (param->attr >= __ARR_SIZE(s_attr_to_type_tbl))
    {
        sh_echo(s_sh_hdl, "param->attr 的值不在 __type_attr_t 所定义的范围\r\n");
        return -1;
    }
    if (s_attr_to_type_tbl[param->attr] != _PARSE_TYPE_STRING)
    {
        sh_echo(s_sh_hdl, "待设置的参数类型不是字符串\r\n");
        return -1;
    }

    if (argv[0] == NULL || *argv[0] == '\0')
    {
        sh_echo(s_sh_hdl, "缺少参数\r\n");
        return -1;
    }

    if (strcmp(argv[0], "?") == 0)
    {
        sh_echo(s_sh_hdl, "最大长度: %d\r\n", bufsize - 1);
        sh_echo(s_sh_hdl, "当前值:   %s\r\n", _get_string(param));
        return 0;
    }

    if (argv[1] && *argv[1] != '\0')
    {
        sh_echo(s_sh_hdl, "参数中有空格。如果目标设备名中包含空格，请使用引号表示\r\n");
        return -1;
    }

    return _set_dest_string(param, argv[0], bufsize);
}

int vset_enum(const __vset_param_t *param, const char *argv[], const char *enum_str)
{
    SYS_ASSERT(s_sh_hdl, "未使用 vset_init() 初始化");
    SYS_ASSERT(param->attr < __ARR_SIZE(s_attr_to_type_tbl), "param->attr 的值不在 __type_attr_t 所定义的范围");

    if (s_sh_hdl == NULL)
    {
        return -1;
    }
    if (param->attr >= __ARR_SIZE(s_attr_to_type_tbl))
    {
        sh_echo(s_sh_hdl, "param->attr 的值不在 __type_attr_t 所定义的范围\r\n");
        return -1;
    }

    if (argv[0] == NULL || *argv[0] == '\0')
    {
        sh_echo(s_sh_hdl, "缺少参数\r\n");
        return -1;
    }

    /* 格式化 enum_str 到 enum_buf */
    char enum_buf[CONFIG_SH_MAX_LINE_LEN];
    int enum_len = _enum_format(enum_buf, __ARR_SIZE(enum_buf), enum_str);
    if (enum_len < 0)
    {
        return -1;
    }

    char *match_key = NULL;
    char *match_value = NULL;
    int type_flag = 0;

    if (strcmp(argv[0], "?") == 0)
    {
        sh_echo(s_sh_hdl, "可选值:\r\n");
        _get_enum_by_param(param, &match_key, &match_value, enum_buf, true, false);
        if (match_value)
        {
            sh_echo(s_sh_hdl, "当前值:\t%s\r\n", match_key);
        }
        return 0;
    }

    type_flag = _get_enum_by_key(argv[0], &match_value, enum_buf);
    if (type_flag == 0)
    {
        sh_echo(s_sh_hdl, "选项为空\r\n");
        return -1;
    }
    if (match_value == NULL)
    {
        sh_echo(s_sh_hdl, "'%s': 非法的取值\r\n", argv[0]);
        return -1;
    }

    return _set_dest_enum(param, match_value, type_flag);
}

void vset_cp_enum(int argc, bool flag, const char *enum_str)
{
    if (argc + flag == 1)
    {
        if (enum_str != NULL)
        {
            SYS_ASSERT(s_sh_hdl, "未使用 vset_init() 初始化");

            if (s_sh_hdl == NULL)
            {
                return;
            }

            /* 格式化 enum_str 到 enum_buf */
            char enum_buf[CONFIG_SH_MAX_LINE_LEN];
            int enum_len = _enum_format(enum_buf, __ARR_SIZE(enum_buf), enum_str);
            if (enum_len < 0)
            {
                return;
            }

            __vset_param_t param = {
                .dest = enum_buf,
                .attr = __GENERIC_ATTR(enum_buf),
            };

            /* 分割字符串 str_buf 并穷举匹配到的值，输出 match_key, match_value, type_flag */
            char *match_key = NULL;
            char *match_value = NULL;
            _get_enum_by_param(&param, &match_key, &match_value, enum_buf, false, true);
        }
        else
        {
            sh_completion_resource(s_sh_hdl, NULL, "? ", NULL);
        }
    }
}

/**
 * @brief 内部函数，由 vset_option_cp() 专用，测试前正在输入的参数有对应存在的候选选项
 *
 * @param input     当前正在输入的参数
 * @param p         选项描述结构体
 * @param size      选项描述结构体的长度
 * @retval true 当前正在输入的参数存在于候选选项中
 * @retval false 当前正在输入的参数不在候选选项中
 */
static bool _is_in_option(const char *input, const sh_vset_param_t *p, unsigned size)
{
    int len = strlen(input);
    for (int i = 0; i < size / sizeof(*p); i++)
    {
        if (strncmp(input, p[i].option, len) == 0)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief 内部函数，由 vset_option_cp() 专用，查找输入的参数的对应选项
 * @brief
 *
 * @param input
 * @param p
 * @param size
 * @return const sh_vset_param_t*
 */
static const sh_vset_param_t *_find_option(const char *input, const sh_vset_param_t *p, unsigned size)
{
    for (int i = 0; i < size / sizeof(*p); i++)
    {
        if (strcmp(input, p[i].option) == 0)
        {
            return &p[i];
        }
    }
    return NULL;
}

/**
 * @brief 内部函数，由 vset_option_cp() 专用，执行对选项的补全
 *
 * @param sh_hdl    原参数
 * @param argc      原参数
 * @param argv      原参数指针值，可能被更新
 * @param p         选项描述结构体
 * @param size      选项描述结构体的长度
 * @retval true
 */
static bool _do_completing_option(sh_t *sh_hdl, int argc, const char *argv[], const sh_vset_param_t *p, unsigned size)
{
    bool ret = false;

    for (int i = 0; i < size / sizeof(*p); i++) // 遍历 p[i].option 分析
    {
        /* 遍历已输入的参数确认当前的 p[i].option 未曾出现过 */
        bool repeat_flag = false;
        for (int j = 0; j < argc - 1; j++)
        {
            if (strcmp(p[i].option, argv[j]) == 0)
            {
                repeat_flag = true;
                break;
            }
        }

        if (repeat_flag == false)
        {
            /* 执行 sh_completion_resource() */
            char option_cpy[0x100];
            int len = 0;
            for (len = 0; len < sizeof(option_cpy) - 2; len++)
            {
                option_cpy[len] = p[i].option[len];
                if (option_cpy[len] == '\0')
                {
                    break;
                }
            }
            option_cpy[len++] = ' ';
            option_cpy[len] = '\0';
            sh_completion_resource(sh_hdl, NULL, option_cpy, p[i].help);
            ret = true;
        }
    }

    return ret;
}

/**
 * @brief 内部函数，由 vset_option_cp() 专用，执行对 SET_ENUM 中的 ENUM_STR 的补全
 *
 * @param option    已知的选项
 * @param p         选项描述结构体
 * @param size      选项描述结构体的长度
 */
static bool _do_completing_enum(const char *option, const sh_vset_param_t *p, unsigned size)
{
    /* 根据上一个参数，遍历 p 找到对应的成员 */
    if (option)
    {
        for (int i = 0; i < size / sizeof(*p); i++)
        {
            if (strcmp(option, p[i].option) == 0)
            {
                if (p[i].set_func)
                {
                    vset_cp_enum(1, false, p[i].enum_str);
                }

                return true;
            }
        }
    }

    return false;
}

static int _set_dest_unsigned(const __vset_param_t *param, unsigned int value, unsigned int low, unsigned int high)
{
    void *dest = param->dest;
    __type_attr_t attr = param->attr;
    vset_global_cb cb = NULL;
    int ret = -1;

    SYS_ASSERT(attr < __ARR_SIZE(s_attr_to_type_tbl), "attr 的值不在 __type_attr_t 所定义的范围");
    SYS_ASSERT(s_attr_to_type_tbl[attr] == _PARSE_TYPE_UNSIGNED, "待设置的参数类型不是正整数");

    if (value < low)
    {
        sh_echo(s_sh_hdl, "设置失败: %d < %d(min)\r\n", value, low);
        return -1;
    }

    if (value > high)
    {
        sh_echo(s_sh_hdl, "设置失败: %d > %d(max)\r\n", value, high);
        return -1;
    }

    switch (attr)
    {
    case __TYPE_ATTR_U8:
    {
        SYS_ASSERT((low & (~0u << 8)) == 0 && (high & (~0u << 8)) == 0, "范围值溢出");
        if ((low & (~0u << 8)) || (high & (~0u << 8)))
        {
            sh_echo(s_sh_hdl, "设置失败: 设置值溢出\r\n");
            return -1;
        }
        if (*((uint8_t *)dest) != value)
        {
            _PARAM_CB();
            *((uint8_t *)dest) = value;
            cb = s_global_cb;
        }
        ret = 0;
        break;
    }

    case __TYPE_ATTR_U16:
    {
        SYS_ASSERT((low & (~0u << 16)) == 0 && (high & (~0u << 16)) == 0, "范围值溢出");
        if ((low & (~0u << 16)) || (high & (~0u << 16)))
        {
            sh_echo(s_sh_hdl, "设置失败: 设置值溢出\r\n");
            return -1;
        }
        if (*((uint16_t *)dest) != value)
        {
            _PARAM_CB();
            *((uint16_t *)dest) = value;
            cb = s_global_cb;
        }
        ret = 0;
        break;
    }

    case __TYPE_ATTR_U32:
    {
        if (*((uint32_t *)dest) != value)
        {
            _PARAM_CB();
            *((uint32_t *)dest) = value;
            cb = s_global_cb;
        }
        ret = 0;
        break;
    }

    case __TYPE_ATTR_U64:
    {
        if (*((uint64_t *)dest) != value)
        {
            _PARAM_CB();
            *((uint64_t *)dest) = value;
            cb = s_global_cb;
        }
        ret = 0;
        break;
    }

    default:
        return -1;
    }

    if (cb)
    {
        cb(s_sh_hdl);
    }
    return ret;
}

static int _set_dest_integer(const __vset_param_t *param, signed int value, signed int low, signed int high)
{
#define _MAX_S8 127
#define _MIN_S8 -128
#define _MAX_S16 32767
#define _MIN_S16 -32768

    void *dest = param->dest;
    __type_attr_t attr = param->attr;
    vset_global_cb cb = NULL;
    int ret = -1;

    SYS_ASSERT(attr < __ARR_SIZE(s_attr_to_type_tbl), "attr 的值不在 __type_attr_t 所定义的范围");
    SYS_ASSERT(s_attr_to_type_tbl[attr] == _PARSE_TYPE_INTEGER, "待设置的参数类型不是带符号整数");

    if (value < low)
    {
        sh_echo(s_sh_hdl, "设置失败: %d < %d(min)\r\n", value, low);
        return -1;
    }

    if (value > high)
    {
        sh_echo(s_sh_hdl, "设置失败: %d > %d(max)\r\n", value, high);
        return -1;
    }

    switch (attr)
    {
    case __TYPE_ATTR_CHR:
    {
        char data = value;
        if (data != value)
        {
            sh_echo(s_sh_hdl, "设置失败: 设置值溢出\r\n");
            return -1;
        }
        if (*((char *)dest) != data)
        {
            _PARAM_CB();
            *((char *)dest) = data;
            cb = s_global_cb;
        }
        ret = 0;
        break;
    }

    case __TYPE_ATTR_BOOL:
    {
        bool data = value;
        if (data != value)
        {
            sh_echo(s_sh_hdl, "设置失败: 设置值溢出\r\n");
            return -1;
        }
        if (*((bool *)dest) != data)
        {
            _PARAM_CB();
            *((bool *)dest) = data;
            cb = s_global_cb;
        }
        ret = 0;
        break;
    }

    case __TYPE_ATTR_S8:
    {
        SYS_ASSERT(low >= _MIN_S8 && low <= _MAX_S8, "范围值溢出");
        SYS_ASSERT(high >= _MIN_S8 && high <= _MAX_S8, "范围值溢出");
        if (low < _MIN_S8 || low > _MAX_S8 ||
            high < _MIN_S8 || high > _MAX_S8)
        {
            sh_echo(s_sh_hdl, "设置失败: 设置值溢出\r\n");
            return -1;
        }
        if (*((int8_t *)dest) != value)
        {
            _PARAM_CB();
            *((int8_t *)dest) = value;
            cb = s_global_cb;
        }
        ret = 0;
        break;
    }

    case __TYPE_ATTR_S16:
    {
        SYS_ASSERT(low >= _MIN_S16 && low <= _MAX_S16, "范围值溢出");
        SYS_ASSERT(high >= _MIN_S16 && high <= _MAX_S16, "范围值溢出");
        if (low < _MIN_S16 || low > _MAX_S16 ||
            high < _MIN_S16 || high > _MAX_S16)
        {
            sh_echo(s_sh_hdl, "设置失败: 设置值溢出\r\n");
            return -1;
        }
        if (*((int16_t *)dest) != value)
        {
            _PARAM_CB();
            *((int16_t *)dest) = value;
            cb = s_global_cb;
        }
        ret = 0;
        break;
    }

    case __TYPE_ATTR_S32:
    {
        if (*((int32_t *)dest) != value)
        {
            _PARAM_CB();
            *((int32_t *)dest) = value;
            cb = s_global_cb;
        }
        ret = 0;
        break;
    }

    case __TYPE_ATTR_S64:
    {
        if (*((int64_t *)dest) != value)
        {
            _PARAM_CB();
            *((int64_t *)dest) = value;
            cb = s_global_cb;
        }
        ret = 0;
        break;
    }

    default:
        return -1;
    }

    if (cb)
    {
        cb(s_sh_hdl);
    }
    return ret;
}

static int _set_dest_float(const __vset_param_t *param, float value, float low, float high)
{
    void *dest = param->dest;
    __type_attr_t attr = param->attr;
    vset_global_cb cb = NULL;
    int ret = -1;

    SYS_ASSERT(attr < __ARR_SIZE(s_attr_to_type_tbl), "attr 的值不在 __type_attr_t 所定义的范围");
    SYS_ASSERT(s_attr_to_type_tbl[attr] == _PARSE_TYPE_FLOAT, "待设置的参数类型不是浮点数");

    if (value < low)
    {
        sh_echo(s_sh_hdl, "设置失败: %f < %f(min)\r\n", value, low);
        return -1;
    }

    if (value > high)
    {
        sh_echo(s_sh_hdl, "设置失败: %f > %f(max)\r\n", value, high);
        return -1;
    }

    switch (attr)
    {
    case __TYPE_ATTR_FLOAT:
    {
        if (*((float *)dest) != value)
        {
            _PARAM_CB();
            *((float *)dest) = value;
            cb = s_global_cb;
        }
        ret = 0;
        break;
    }
    case __TYPE_ATTR_DOUBLE:
    {
        if (*((double *)dest) != value)
        {
            _PARAM_CB();
            *((double *)dest) = value;
            cb = s_global_cb;
        }
        ret = 0;
        break;
    }

    default:
        return -1;
    }

    if (cb)
    {
        cb(s_sh_hdl);
    }
    return ret;
}

static int _set_dest_string(const __vset_param_t *param, const char *str, unsigned bufsize)
{
    void *dest = param->dest;
    int str_size = strlen(str) + 1;
    __type_attr_t attr = param->attr;
    vset_global_cb cb = NULL;
    int ret = -1;

    SYS_ASSERT(attr < __ARR_SIZE(s_attr_to_type_tbl), "attr 的值不在 __type_attr_t 所定义的范围");
    SYS_ASSERT(s_attr_to_type_tbl[attr] == _PARSE_TYPE_STRING, "待设置的参数类型不是字符串");

    if (str_size > bufsize)
    {
        sh_echo(s_sh_hdl, "注意: '%s' 大于最大长度 %d\r\n", str, bufsize - 1);
    }

    switch (attr)
    {
    case __TYPE_ATTR_STR:
        memset(dest, 0, bufsize);
        if (memcmp(dest, str, str_size))
        {
            if (param->cb)
            {
                char value[0x100];
                memcpy(dest, str, str_size);
                _PARAM_CB();
            }
            memcpy(dest, str, str_size);
            cb = s_global_cb;
        }
        ret = 0;
        break;

    default:
        return -1;
    }

    if (cb)
    {
        cb(s_sh_hdl);
    }
    return ret;
}

static int _set_dest_enum(const __vset_param_t *param, const char *value, int type_flag)
{
    sh_parse_t pv = sh_parse_value(value);
    switch (s_attr_to_type_tbl[param->attr])
    {
    case _PARSE_TYPE_UNSIGNED: // 待设置的参数格式是无符号整型
    {
        if (type_flag & (_TYPE_FLAG_INTEGER | _TYPE_FLAG_FLOAT | _TYPE_FLAG_STRING))
        {
            sh_echo(s_sh_hdl, "ENUM_STR 格式错误, 包含非法的 value 类型\r\n");
            return -1;
        }
        switch (pv.type)
        {
        case _PARSE_TYPE_UNSIGNED: // 当前匹配的参数格式是无符号整型
            return _set_dest_unsigned(param, pv.value.val_unsigned, pv.value.val_unsigned, pv.value.val_unsigned);

        default:
            return -1;
        }
    }
    break;

    case _PARSE_TYPE_INTEGER: // 待设置的参数格式是带符号整型
    {
        if (type_flag & (_TYPE_FLAG_FLOAT | _TYPE_FLAG_STRING))
        {
            sh_echo(s_sh_hdl, "ENUM_STR 格式错误, 包含非法的 value 类型\r\n");
            return -1;
        }
        switch (pv.type)
        {
        case _PARSE_TYPE_INTEGER: // 当前匹配的参数格式是带符号整型
            return _set_dest_integer(param, pv.value.val_integer, pv.value.val_integer, pv.value.val_integer);

        case _PARSE_TYPE_UNSIGNED: // 当前匹配的参数格式是无符号整型
            return _set_dest_integer(param, pv.value.val_unsigned, pv.value.val_unsigned, pv.value.val_unsigned);

        default:
            return -1;
        }
    }
    break;

    case _PARSE_TYPE_FLOAT: // 待设置的参数格式是浮点数
    {
        if (type_flag & (_TYPE_FLAG_STRING))
        {
            sh_echo(s_sh_hdl, "ENUM_STR 格式错误, 包含非法的 value 类型\r\n");
            return -1;
        }
        switch (pv.type)
        {
        case _PARSE_TYPE_INTEGER: // 当前匹配的参数格式是带符号整型
            return _set_dest_float(param, pv.value.val_integer, pv.value.val_integer, pv.value.val_integer);

        case _PARSE_TYPE_UNSIGNED: // 当前匹配的参数格式是无符号整型
            return _set_dest_float(param, pv.value.val_unsigned, pv.value.val_unsigned, pv.value.val_unsigned);

        case _PARSE_TYPE_FLOAT: // 当前匹配的参数格式是浮点数
            return _set_dest_float(param, pv.value.val_float, pv.value.val_float, pv.value.val_float);

        default:
            return -1;
        }
    }
    break;

    case _PARSE_TYPE_STRING: // 待设置的参数格式是字符串
    {
        return _set_dest_string(param, value, strlen(value) + 1);
    }
    break;

    default:
        sh_echo(s_sh_hdl, "param->attr 的值不在 __type_attr_t 所定义的范围\r\n");
        return -1;
    }

    return -1;
}

static signed int _get_integer(const __vset_param_t *param)
{
    SYS_ASSERT(s_sh_hdl, "未使用 vset_init() 初始化");
    SYS_ASSERT(param->attr < __ARR_SIZE(s_attr_to_type_tbl), "param->attr 的值不在 __type_attr_t 所定义的范围");
    SYS_ASSERT(s_attr_to_type_tbl[param->attr] == _PARSE_TYPE_INTEGER, "待设置的参数类型不是带符号整数");

    switch (param->attr)
    {
    case __TYPE_ATTR_CHR:
        return *((char *)param->dest);

    case __TYPE_ATTR_BOOL:
        return *((bool *)param->dest);

    case __TYPE_ATTR_S8:
        return *((int8_t *)param->dest);

    case __TYPE_ATTR_S16:
        return *((int16_t *)param->dest);

    case __TYPE_ATTR_S32:
        return *((int32_t *)param->dest);

    case __TYPE_ATTR_S64:
        return *((int64_t *)param->dest);

    default:
        return -1;
    }
}

static float _get_float(const __vset_param_t *param)
{
    SYS_ASSERT(s_sh_hdl, "未使用 vset_init() 初始化");
    SYS_ASSERT(param->attr < __ARR_SIZE(s_attr_to_type_tbl), "param->attr 的值不在 __type_attr_t 所定义的范围");
    SYS_ASSERT(s_attr_to_type_tbl[param->attr] == _PARSE_TYPE_FLOAT, "待设置的参数类型不是浮点数");

    switch (param->attr)
    {
    case __TYPE_ATTR_FLOAT:
        return *((float *)param->dest);

    case __TYPE_ATTR_DOUBLE:
        return *((double *)param->dest);

    default:
        return -1;
    }
}

static char *_get_string(const __vset_param_t *param)
{
    SYS_ASSERT(s_sh_hdl, "未使用 vset_init() 初始化");
    SYS_ASSERT(param->attr < __ARR_SIZE(s_attr_to_type_tbl), "param->attr 的值不在 __type_attr_t 所定义的范围");
    SYS_ASSERT(s_attr_to_type_tbl[param->attr] == _PARSE_TYPE_STRING, "待设置的参数类型不是字符串");

    return param->dest;
}

static unsigned int _get_unsigned(const __vset_param_t *param)
{
    SYS_ASSERT(s_sh_hdl, "未使用 vset_init() 初始化");
    SYS_ASSERT(param->attr < __ARR_SIZE(s_attr_to_type_tbl), "param->attr 的值不在 __type_attr_t 所定义的范围");
    SYS_ASSERT(s_attr_to_type_tbl[param->attr] == _PARSE_TYPE_UNSIGNED, "待设置的参数类型不是正整数");

    switch (param->attr)
    {
    case __TYPE_ATTR_U8:
        return *((uint8_t *)param->dest);

    case __TYPE_ATTR_U16:
        return *((uint16_t *)param->dest);

    case __TYPE_ATTR_U32:
        return *((uint32_t *)param->dest);

    case __TYPE_ATTR_U64:
        return *((uint64_t *)param->dest);

    default:
        return -1;
    }
}

/**
 * @brief 格式化原始的 enum_str 并输出到  enum_buf
 *
 * @param enum_buf[out] 复制 enum_str 并格式化的需要的缓存
 * @param buf_len       enum_buf 的长度
 * @param enum_str      原始的选项格式
 * @retval < 0 表示 enum_str 的格式错误(键值或数值不能包含空格)
 * @retval >= 0 表示输出的 enum_buf 中的字符数（不包含 '\0'）
 */
static int _enum_format(char *enum_buf, int buf_len, const char *enum_str)
{
    /* 复制 enum_str 到 str_buf 并移除空格 */
    if (strlen(enum_str) >= buf_len)
    {
        sh_echo(s_sh_hdl, "注意: str_buff 长度不足\r\n");
    }
    strncpy(enum_buf, enum_str, buf_len - 1);
    enum_buf[buf_len - 1] = '\0';

    /* 去除可能的多余空格 */
    buf_len = strlen(enum_buf);
    int cid = 0;
    int flag1 = 0;
    int flag2 = 1;
    for (int i = 0; i < buf_len; i++)
    {
        if (enum_buf[i] > ' ')
        {
            enum_buf[cid] = enum_str[i];
            cid++;

            if (enum_buf[i] != ',' && enum_buf[i] != '=')
            {
                flag1 += flag2;
                flag1 += !flag1;
                if (flag1 > 1)
                {
                    break;
                }
            }
            else
            {
                flag1 = 0;
            }
            flag2 = 0;
        }
        else
        {
            flag2 = 1;
        }
    }
    if (flag1 > 1)
    {
        sh_echo(s_sh_hdl, "ENUM_STR 格式错误, 键值或数值不能包含空格\r\n");
        return -1;
    }

    enum_buf[cid] = '\0';
    return cid; // 返回字符串长度（不包含结束符）
}

/**
 * @brief 根据给出的 key 在 enum_buf 查找对应的 value
 *
 * @param key               给出的被搜索的 key
 * @param match_value[out]  与 key 中的值完全匹配的 value 位置，该位置在 enum_buf 中，如果结果为 NULL 表示在 enum_buf 中不存在与 param 的值完全匹配的项
 * @param enum_buf          通过 _enum_format() 格式化的缓存
 * @retval int 按位表示在 enum_buf 中出现过的 value 的数据类型。
 *      _TYPE_FLAG_UNSIGNED     掩码表示在 enum_buf 中包含无符号整型
 *      _TYPE_FLAG_INTEGER      掩码表示在 enum_buf 中包含有符号整型
 *      _TYPE_FLAG_FLOAT        掩码表示在 enum_buf 中包含浮点数
 *      _TYPE_FLAG_STRING       掩码表示在 enum_buf 中包含字符串
 */
static int _get_enum_by_key(const char *key, char **match_value, char *enum_buf)
{
    int type_flag = 0; // 在 match_value 中出现过的数据类型

    /* 分割字符串 enum_buf 并穷举匹配到的值 */
    *match_value = NULL;
    char *save_token;
    char *token = strtok_r(enum_buf, ",", &save_token);
    while (token)
    {
        char *enum_value;
        char *enum_key = strtok_r(token, "=", &enum_value);

        if (enum_key == NULL || *enum_key == '\0' || *enum_value == '\0')
        {
            sh_echo(s_sh_hdl, "'%s' 的格式错误, 请使用如下格式表示: 'key1=value,key2=value,...'\r\n", token);
            return 0;
        }

        sh_parse_t pv = sh_parse_value(enum_value);
        switch (pv.type)
        {
        case _PARSE_TYPE_INTEGER: // 解析出的参数格式是带符号整型
            type_flag |= _TYPE_FLAG_INTEGER;
            break;

        case _PARSE_TYPE_UNSIGNED: // 解析出的参数格式是无符号整型
            type_flag |= _TYPE_FLAG_UNSIGNED;
            break;

        case _PARSE_TYPE_FLOAT: // 解析出的参数格式是浮点数
            type_flag |= _TYPE_FLAG_FLOAT;
            break;

        case _PARSE_TYPE_STRING: // 解析出的参数格式是字符串
            type_flag |= _TYPE_FLAG_STRING;
            break;

        default:
            break;
        }

        if (*match_value == NULL)
        {
            if (strcmp(enum_key, key) == 0)
            {
                *match_value = enum_value;
            }
        }

        token = strtok_r(NULL, ",", &save_token);
    }

    return type_flag;
}

/**
 * @brief 根据给出的 param 在 enum_buf 查找对应的 key 和 value
 * 1. 分析 enum_buf 中存在的所有 value 项的数据类型并作为返回值
 * 2. 根据 param 的值，在格式化数据中搜索与该值完全匹配的 key 和 value 并输出到 *match_key 和 *match_value
 * 2. 可选打印 enum_buf 中的 key 和 value
 *
 * @param param             指向待设置的变量属性的结构体
 * @param match_key[out]    与 param 中的值完全匹配的 key 位置，该位置在 enum_buf 中，如果结果为 NULL 表示在 enum_buf 中不存在与 param 的值完全匹配的项
 * @param match_value[out]  与 param 中的值完全匹配的 value 位置，该位置在 enum_buf 中，如果结果为 NULL 表示在 enum_buf 中不存在与 param 的值完全匹配的项
 * @param enum_buf          通过 _enum_format() 格式化的缓存
 * @param print_list        是否打印所有 enum_buf 中的项目
 * @param print_cp          是否执行自动补全
 * @retval int 按位表示在 enum_buf 中出现过的 value 的数据类型。
 *      _TYPE_FLAG_UNSIGNED     掩码表示在 enum_buf 中包含无符号整型
 *      _TYPE_FLAG_INTEGER      掩码表示在 enum_buf 中包含有符号整型
 *      _TYPE_FLAG_FLOAT        掩码表示在 enum_buf 中包含浮点数
 *      _TYPE_FLAG_STRING       掩码表示在 enum_buf 中包含字符串
 */
static int _get_enum_by_param(const __vset_param_t *param, char **match_key, char **match_value, char *enum_buf, bool print_list, bool print_cp)
{
    char param_value_str[0x100];
    int type_flag = 0; // 在 match_value 中出现过的数据类型

    if (param->attr >= __ARR_SIZE(s_attr_to_type_tbl))
    {
        sh_echo(s_sh_hdl, "param->attr 的值不在 __type_attr_t 所定义的范围\r\n");
        return -1;
    }

    switch (s_attr_to_type_tbl[param->attr])
    {
    case _PARSE_TYPE_INTEGER: // 待设置的参数格式是带符号整型
        SYS_SNPRINT(param_value_str, sizeof(param_value_str), "%d", _get_integer(param));
        break;

    case _PARSE_TYPE_UNSIGNED: // 待设置的参数格式是无符号整型
        SYS_SNPRINT(param_value_str, sizeof(param_value_str), "%u", _get_unsigned(param));
        break;

    case _PARSE_TYPE_FLOAT: // 待设置的参数格式是浮点数
        SYS_SNPRINT(param_value_str, sizeof(param_value_str), "%f", _get_float(param));
        break;

    case _PARSE_TYPE_STRING: // 待设置的参数格式是字符串
        SYS_SNPRINT(param_value_str, sizeof(param_value_str), "%s", _get_string(param));
        break;

    default:
        sh_echo(s_sh_hdl, "param->attr 的值不在 __type_attr_t 所定义的范围\r\n");
        return -1;
    }
    param_value_str[__ARR_SIZE(param_value_str) - 1] = '\0';

    /* 分割字符串 enum_buf 并穷举匹配到的值 */
    *match_key = NULL;
    *match_value = NULL;
    char *save_token;
    char *token = strtok_r(enum_buf, ",", &save_token);
    while (token)
    {
        char *enum_value;
        char *enum_key = strtok_r(token, "=", &enum_value);
        char key_value_str[__ARR_SIZE(param_value_str)];

        if (*enum_key == '\0' || *enum_value == '\0')
        {
            sh_echo(s_sh_hdl, "'%s' 的格式错误, 请使用如下格式表示: 'key1=value,key2=value,...'\r\n", enum_key);
            return 0;
        }

        key_value_str[0] = '\0';
        sh_parse_t pv = sh_parse_value(enum_value);
        switch (pv.type)
        {
        case _PARSE_TYPE_UNSIGNED: // 解析出的参数格式是无符号整型
            type_flag |= _TYPE_FLAG_UNSIGNED;
            if (*match_key == NULL)
            {
                switch (s_attr_to_type_tbl[param->attr])
                {
                case _PARSE_TYPE_UNSIGNED: // 待设置的参数格式是无符号整型
                    SYS_SNPRINT(key_value_str, sizeof(key_value_str), "%u", pv.value.val_unsigned);
                    break;
                case _PARSE_TYPE_INTEGER: // 待设置的参数格式是带符号整型
                    SYS_SNPRINT(key_value_str, sizeof(key_value_str), "%d", pv.value.val_unsigned);
                    break;
                case _PARSE_TYPE_FLOAT: // 待设置的参数格式是浮点数
                    SYS_SNPRINT(key_value_str, sizeof(key_value_str), "%f", (float)pv.value.val_unsigned);
                    break;
                case _PARSE_TYPE_STRING: // 待设置的参数格式是字符串
                    SYS_SNPRINT(key_value_str, sizeof(key_value_str), "%s", enum_value);
                    break;
                default:
                    break;
                }
            }
            break;

        case _PARSE_TYPE_INTEGER: // 解析出的参数格式是带符号整型
            type_flag |= _TYPE_FLAG_INTEGER;
            if (*match_key == NULL)
            {
                switch (s_attr_to_type_tbl[param->attr])
                {
                case _PARSE_TYPE_INTEGER: // 待设置的参数格式是带符号整型
                    SYS_SNPRINT(key_value_str, sizeof(key_value_str), "%d", pv.value.val_integer);
                    break;
                case _PARSE_TYPE_FLOAT: // 待设置的参数格式是浮点数
                    SYS_SNPRINT(key_value_str, sizeof(key_value_str), "%f", (float)pv.value.val_integer);
                    break;
                case _PARSE_TYPE_STRING: // 待设置的参数格式是字符串
                    SYS_SNPRINT(key_value_str, sizeof(key_value_str), "%s", enum_value);
                    break;
                default:
                    break;
                }
            }
            break;

        case _PARSE_TYPE_FLOAT: // 解析出的参数格式是浮点数
            type_flag |= _TYPE_FLAG_FLOAT;
            if (*match_key == NULL)
            {
                switch (s_attr_to_type_tbl[param->attr])
                {
                case _PARSE_TYPE_FLOAT: // 待设置的参数格式是浮点数
                    SYS_SNPRINT(key_value_str, sizeof(key_value_str), "%f", (float)pv.value.val_float);
                    break;
                case _PARSE_TYPE_STRING: // 待设置的参数格式是字符串
                    SYS_SNPRINT(key_value_str, sizeof(key_value_str), "%s", enum_value);
                    break;
                default:
                    break;
                }
            }
            break;

        case _PARSE_TYPE_STRING: // 解析出的参数格式是字符串
            type_flag |= _TYPE_FLAG_STRING;
            if (*match_key == NULL)
                SYS_SNPRINT(key_value_str, sizeof(key_value_str), "%s", enum_value);
            break;

        default:
            break;
        }

        if (*match_key == NULL)
        {
            if (strcmp(key_value_str, param_value_str) == 0)
            {
                *match_key = enum_key;
                *match_value = enum_value;
            }
        }

        if (print_list)
        {
            sh_echo(s_sh_hdl, "\t%s (%s)\r\n", enum_key, enum_value);
        }

        if (print_cp)
        {
            char key_cpy[0x100];
            int len = 0;
            for (len = 0; len < sizeof(key_cpy) - 2; len++)
            {
                key_cpy[len] = enum_key[len];
                if (key_cpy[len] == '\0')
                {
                    break;
                }
            }
            key_cpy[len++] = ' ';
            key_cpy[len] = '\0';
            sh_completion_resource(s_sh_hdl, NULL, key_cpy, NULL);
        }

        token = strtok_r(NULL, ",", &save_token);
    }

    switch (s_attr_to_type_tbl[param->attr])
    {
    case _PARSE_TYPE_UNSIGNED: // 待设置的参数格式是无符号整型
    {
        if (type_flag & (_TYPE_FLAG_INTEGER | _TYPE_FLAG_FLOAT | _TYPE_FLAG_STRING))
        {
            sh_echo(s_sh_hdl, "ENUM_STR 格式错误, 包含非法的 value 类型\r\n");
            return -1;
        }
    }
    break;

    case _PARSE_TYPE_INTEGER: // 待设置的参数格式是带符号整型
    {
        if (type_flag & (_TYPE_FLAG_FLOAT | _TYPE_FLAG_STRING))
        {
            sh_echo(s_sh_hdl, "ENUM_STR 格式错误, 包含非法的 value 类型\r\n");
            return -1;
        }
    }
    break;

    case _PARSE_TYPE_FLOAT: // 待设置的参数格式是浮点数
    {
        if (type_flag & (_TYPE_FLAG_STRING))
        {
            sh_echo(s_sh_hdl, "ENUM_STR 格式错误, 包含非法的 value 类型\r\n");
            return -1;
        }
    }
    break;

    default:
        break;
    }

    return type_flag;
}
