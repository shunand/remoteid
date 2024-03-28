/**
 * @file sh_vset.h
 * @author LokLiang
 * @brief shell 模块专用，可用于通过命令行中的参数设置变量的基本接口
 * @version 0.1
 * @date 2023-09-22
 *
 * @copyright Copyright (c) 2023
 *
 * 本模块实现解析字符，并根据解析的结果对常见类型的的变量赋值的功能。
 *
 * 特性：
 * - 自动分析设置变量的具体类型
 * - 对取值范围限制
 * - 可通过命令行的形式进行设置
 * - 每增加一项写入接口最低消耗 38 个字节的代码空间
 *
 * 使用：
 * 1. 使用 vset_init() 指定内部的 sh 模块对象
 * 2. 使用对应的宏作为设置函数，其中：
 * - @b SET_VAR     用于设置无符号整形、带符号整型、单精度的浮点数和字符串的变量
 * - @b SET_ENUM    用于设置枚举型的变量，支持 SET_VAR 所支持的全部类型
 * - @b SET_CP_ENUM 对应 SET_ENUM 所设置的变量的可用自动补全函数
 * 3. 如果输入的参数为 ? 可用于打印当前的设置范围。
 * 4. 通过选项设置参数，其中：
 * - @b PSET_FN     指定一个已定义的 const sh_vset_param_t* 的结构作为选项
 * - @b PSET_CP     根据一个已定义的 const sh_vset_param_t* 执行自动补选项
 *
 * 实例1: 以下这些函数可被 SH_SETUP_CMD 中定义的 FUNC 执行，或者作为 sh_vset_param_t::set_func 的成员
 * static int _value_set_u8(const char *argv[])    { SET_VAR(&var_u8, 1, 200); }
 * static int _value_set_s16(const char *argv[])   { SET_VAR(&var_s16, -1000, 1000); }
 * static int _value_set_float(const char *argv[]) { SET_VAR(&var_float, -1000, var_u8); }
 * static int _value_set_str(const char *argv[])   { SET_VAR(&var_str, 0, 0); }
 * static int _value_set_enum(const char *argv[])  { SET_ENUM(&var_s32, "enable=1,disable=0"); }
 *
 * 实例2: 实现选项+参数的格式
 * static sh_vset_param_t const s_param_template[] = {
 *     {"--u8", "该选项的帮助信息", value_set_u8, NULL},
 *     {"--s16", "该选项的帮助信息", value_set_s16, NULL},
 *     {"--float", "该选项的帮助信息", value_set_float, NULL},
 *     {"--str", "该选项的帮助信息", value_set_str, NULL},
 *     {"--enum", "该选项的帮助信息", value_set_enum, "enable=1,disable=0"},
 * static int _value_set(sh_t *sh_hdl, int argc, const char *argv[])            { PSET_FN(s_param_template); } // 这个函数可作为 SH_SETUP_CMD 中定义的 FUNC
 * static void _value_cp(sh_t *sh_hdl, int argc, const char *argv[], bool flag) { PSET_CP(s_param_template); } // 这个函数可作为 SH_SETUP_CMD 中定义的 SUB
 *
 */

#ifndef __SH_VSET_H__
#define __SH_VSET_H__

#include "sys_types.h"
#include "sh.h"

/* 描述支待的待设置变量的具体类型 */
typedef enum
{
    __TYPE_ATTR_CHR,
    __TYPE_ATTR_BOOL,
    __TYPE_ATTR_U8,
    __TYPE_ATTR_S8,
    __TYPE_ATTR_U16,
    __TYPE_ATTR_S16,
    __TYPE_ATTR_U32,
    __TYPE_ATTR_S32,
    __TYPE_ATTR_U64,
    __TYPE_ATTR_S64,
    __TYPE_ATTR_FLOAT,
    __TYPE_ATTR_DOUBLE,
    __TYPE_ATTR_STR,
    __TYPE_ATTR_OTHER,
} __type_attr_t;

/**
 * @brief vset_cb
 * 当一个参数的设置值合法，即将被执行设置前回调的函数。当回调退出后才会更新到目标变量中。
 * @param new_value 指向 sh_vset 内部的新值的栈内存地址。提示：可配合 __typeof() 强制转换为确定的类型。
 */
typedef void (*vset_cb)(sh_t *sh_hdl, void *new_value);

/* 待设置变量数据结构 */
typedef struct
{
    void *dest;
    __type_attr_t attr;
    vset_cb cb;
} __vset_param_t;

#define __GENERIC_ATTR(VAR) (__builtin_types_compatible_p(__typeof(VAR), char)                ? __TYPE_ATTR_CHR    \
                             : __builtin_types_compatible_p(__typeof(VAR), volatile char)     ? __TYPE_ATTR_CHR    \
                             : __builtin_types_compatible_p(__typeof(VAR), bool)              ? __TYPE_ATTR_BOOL   \
                             : __builtin_types_compatible_p(__typeof(VAR), volatile bool)     ? __TYPE_ATTR_BOOL   \
                             : __builtin_types_compatible_p(__typeof(VAR), uint8_t)           ? __TYPE_ATTR_U8     \
                             : __builtin_types_compatible_p(__typeof(VAR), volatile uint8_t)  ? __TYPE_ATTR_U8     \
                             : __builtin_types_compatible_p(__typeof(VAR), int8_t)            ? __TYPE_ATTR_S8     \
                             : __builtin_types_compatible_p(__typeof(VAR), volatile int8_t)   ? __TYPE_ATTR_S8     \
                             : __builtin_types_compatible_p(__typeof(VAR), uint16_t)          ? __TYPE_ATTR_U16    \
                             : __builtin_types_compatible_p(__typeof(VAR), volatile uint16_t) ? __TYPE_ATTR_U16    \
                             : __builtin_types_compatible_p(__typeof(VAR), int16_t)           ? __TYPE_ATTR_S16    \
                             : __builtin_types_compatible_p(__typeof(VAR), volatile int16_t)  ? __TYPE_ATTR_S16    \
                             : __builtin_types_compatible_p(__typeof(VAR), uint32_t)          ? __TYPE_ATTR_U32    \
                             : __builtin_types_compatible_p(__typeof(VAR), volatile uint32_t) ? __TYPE_ATTR_U32    \
                             : __builtin_types_compatible_p(__typeof(VAR), int32_t)           ? __TYPE_ATTR_S32    \
                             : __builtin_types_compatible_p(__typeof(VAR), volatile int32_t)  ? __TYPE_ATTR_S32    \
                             : __builtin_types_compatible_p(__typeof(VAR), uint64_t)          ? __TYPE_ATTR_U64    \
                             : __builtin_types_compatible_p(__typeof(VAR), volatile uint64_t) ? __TYPE_ATTR_U64    \
                             : __builtin_types_compatible_p(__typeof(VAR), int64_t)           ? __TYPE_ATTR_S64    \
                             : __builtin_types_compatible_p(__typeof(VAR), volatile int64_t)  ? __TYPE_ATTR_S64    \
                             : __builtin_types_compatible_p(__typeof(VAR), float)             ? __TYPE_ATTR_FLOAT  \
                             : __builtin_types_compatible_p(__typeof(VAR), volatile float)    ? __TYPE_ATTR_FLOAT  \
                             : __builtin_types_compatible_p(__typeof(VAR), double)            ? __TYPE_ATTR_DOUBLE \
                             : __builtin_types_compatible_p(__typeof(VAR), volatile double)   ? __TYPE_ATTR_DOUBLE \
                             : __builtin_types_compatible_p(__typeof(VAR), char[])            ? __TYPE_ATTR_STR    \
                                                                                              : __TYPE_ATTR_OTHER)

typedef int (*vset_var_fn)(const char *argv[]);

typedef struct // 用于长选项设置的描述结构
{
    const char *option;   // 选项，如 "--value"
    const char *help;     // 对该选项的描述
    vset_var_fn set_func; // 与 option 对应的，使用宏 SET_VAR() 或 SET_ENUM() 设置变量的函数。如果值为 NULL 表示该选项无参数，同时对应的输入参数被保留
    const char *enum_str; // 仅在类型为 vset_enum_fn 时有效，为对应的选项提供可选的补全参数选项，值为 NULL 或 "" 时默认候选参数为 '?'
} sh_vset_param_t;

int vset_unsigned(const __vset_param_t *param, const char *argv[], unsigned int low, unsigned int high);
int vset_integer(const __vset_param_t *param, const char *argv[], signed int low, signed int high);
int vset_float(const __vset_param_t *param, const char *argv[], float low, float high);
int vset_str(const __vset_param_t *param, const char *argv[], unsigned bufsize);
int vset_enum(const __vset_param_t *param, const char *argv[], const char *enum_str);
void vset_cp_enum(int argc, bool flag, const char *enum_str);

int vset_option_set(sh_t *sh_hdl, int *argc, const char *argv[], const sh_vset_param_t *p, unsigned size);
bool vset_option_cp(sh_t *sh_hdl, int argc, const char *argv[], bool flag, const sh_vset_param_t *p, unsigned size);

/* 自动分析并设置变量的值，带设置回调 */
#define SET_VAR_CB(NAME, LOW, HIGH, CB)                     \
    do                                                      \
    {                                                       \
        static __vset_param_t const param = {               \
            .dest = NAME,                                   \
            .attr = __GENERIC_ATTR(*(NAME)),                \
            .cb = CB,                                       \
        };                                                  \
        if (                                                \
            __GENERIC_ATTR(*(NAME)) == __TYPE_ATTR_U8 ||    \
            __GENERIC_ATTR(*(NAME)) == __TYPE_ATTR_U16 ||   \
            __GENERIC_ATTR(*(NAME)) == __TYPE_ATTR_U32 ||   \
            __GENERIC_ATTR(*(NAME)) == __TYPE_ATTR_U64)     \
        {                                                   \
            return vset_unsigned(&param, argv, LOW, HIGH);  \
        }                                                   \
        else if (                                           \
            __GENERIC_ATTR(*(NAME)) == __TYPE_ATTR_CHR ||   \
            __GENERIC_ATTR(*(NAME)) == __TYPE_ATTR_BOOL ||  \
            __GENERIC_ATTR(*(NAME)) == __TYPE_ATTR_S8 ||    \
            __GENERIC_ATTR(*(NAME)) == __TYPE_ATTR_S16 ||   \
            __GENERIC_ATTR(*(NAME)) == __TYPE_ATTR_S32 ||   \
            __GENERIC_ATTR(*(NAME)) == __TYPE_ATTR_S64)     \
        {                                                   \
            return vset_integer(&param, argv, LOW, HIGH);   \
        }                                                   \
        else if (                                           \
            __GENERIC_ATTR(*(NAME)) == __TYPE_ATTR_FLOAT || \
            __GENERIC_ATTR(*(NAME)) == __TYPE_ATTR_DOUBLE)  \
        {                                                   \
            return vset_float(&param, argv, LOW, HIGH);     \
        }                                                   \
        else if (                                           \
            __GENERIC_ATTR(*(NAME)) == __TYPE_ATTR_STR)     \
        {                                                   \
            return vset_str(&param, argv, sizeof(*(NAME))); \
        }                                                   \
        else                                                \
        {                                                   \
            return -1;                                      \
        }                                                   \
    } while (0)

/* 设置数据类型为 枚举型 的变量，带设置回调  */
#define SET_ENUM_CB(NAME, ENUM_STR, CB)           \
    do                                            \
    {                                             \
        static __vset_param_t const param = {     \
            .dest = NAME,                         \
            .attr = __GENERIC_ATTR(*(NAME)),      \
            .cb = CB,                             \
        };                                        \
        return vset_enum(&param, argv, ENUM_STR); \
    } while (0)

/* 自动分析并设置变量的值 */
#define SET_VAR(NAME, LOW, HIGH) SET_VAR_CB(NAME, LOW, HIGH, NULL)

/* 设置数据类型为 枚举型 的变量 */
#define SET_ENUM(NAME, ENUM_STR) SET_ENUM_CB(NAME, ENUM_STR, NULL)

/* 对应 SET_VAR 所设置的变量的可用自动补全函数 */
#define SET_CP_VAR()                                          \
    do                                                        \
    {                                                         \
        if (argc + flag == 1)                                 \
        {                                                     \
            sh_completion_resource(sh_hdl, NULL, "? ", NULL); \
        }                                                     \
    } while (0)

/* 对应 SET_ENUM 所设置的变量的可用自动补全函数 */
#define SET_CP_ENUM(ENUM_STR)               \
    do                                      \
    {                                       \
        vset_cp_enum(argc, flag, ENUM_STR); \
    } while (0)

#define PSET_FN(OPT) vset_option_set(sh_hdl, &argc, argv, OPT, sizeof(OPT))     /* 作为 SH_CMD_FN() 的实际执行函数 */
#define PSET_CP(OPT) vset_option_cp(sh_hdl, argc, argv, flag, OPT, sizeof(OPT)) /* 作为 SH_CMD_CP_FN() 的实际执行函数 */

typedef void (*vset_global_cb)(sh_t *sh_hdl);

void vset_init(sh_t *sh_hdl, vset_global_cb cb);

void vset_force_cb(void);

#endif
