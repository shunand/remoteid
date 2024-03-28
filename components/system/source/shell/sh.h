/**
 * @file sh.h
 * @author LokLiang (lokliang@163.com)
 * @brief
 * @version 0.1
 * @date 2023-03-21
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __SH_H__
#define __SH_H__

#include "sys_init.h"
#include "list/pslist.h"

#define SH_VERSION "0.1"

#ifndef CONFIG_SH_MAX_LINE_LEN
#define CONFIG_SH_MAX_LINE_LEN 512 /* 最大命令长度 */
#endif

#ifndef CONFIG_SH_USE_STRTOD
#define CONFIG_SH_USE_STRTOD 1 /* 允许参数解析工具 sh_parse_value() 解析浮点数 */
#endif

typedef struct sh_obj_def sh_t;

typedef int (*sh_vprint_fn)(const char *format, va_list va); // 实现发送字符串到终端
typedef void (*sh_disconnect_fn)(void);                      // 实现断开连接

typedef struct // 兼容接口
{
    void (*set_cursor_hoffset)(sh_t *sh_hdl, int last_pos, int new_pos); // 设置光标水平位置
    void (*insert_str)(sh_t *sh_hdl, const char *str);                   // 在光标前插入一个字符串
    sh_vprint_fn vprint;                                                 // 实现发送字符串到终端
    sh_disconnect_fn disconnect;                                         // 实现断开连接

    void (*clear_line)(sh_t *sh_hdl); // 执行清除一行。值为 NULL 时不启用刷命令行
} sh_port_t;

typedef struct
{
    const char *arg_str;                            // 待分析的参数
    int arg_len;                                    // 待解析的参数长度
    char match_str[CONFIG_SH_MAX_LINE_LEN / 4 * 4]; // 保存相同的字符部分
    int list_algin;                                 // 配合 list_fmt ，确定格式中 %-ns 的 n 的值
    int match_num;                                  // 可打印的列表的条目数
    int print_count;                                // 已打印计算
} sh_cp_info_t;

typedef enum __packed
{
    SH_CP_OP_NA,   // 未发生任何动作
    SH_CP_OP_CP,   // 执行了自动补全
    SH_CP_OP_PEND, // 正在列举选项
    SH_CP_OP_LIST, // 完成了列举选项
} sh_cp_op_t;

typedef struct sh_cmd sh_cmd_t;

typedef void (*sh_cp_fn)(sh_t *sh_hdl, int argc, const char *argv[], bool flag); // flag: 表示当前输入的最后一个参数是否完整（以空格分开）

typedef enum
{
    _SH_SUB_TYPE_VOID,
    _SH_SUB_TYPE_CPFN,
    _SH_SUB_TYPE_SUB,
} sh_fn_type_t;

typedef struct sh_param
{
    const char *cmd;
    const char *help;
} sh_cp_param_t;

typedef struct sh_cmd
{
    const char *cmd;
    const char *help;
    int (*cmd_func)(sh_t *sh_hdl, int argc, const char *argv[]);
    union
    {
        const void *nul;
        const sh_cmd_t *sub_cmd;
        sh_cp_fn cp_fn;
    } sub_fn;
    sh_fn_type_t fn_type;
} sh_cmd_t;

typedef struct
{
    sys_psnode_t *node;
    const sh_cmd_t *cmd;
    const char *file;
    int line;
} sh_cmd_reg_t;

typedef struct
{
    const char *code;
    void (*key_func)(sh_t *sh_hdl);
} sh_key_t;

typedef struct
{
    sys_psnode_t *node;
    const sh_key_t *key;
} sh_key_reg_t;

typedef struct
{
    enum
    {
        _SH_CMD_STATUS_Bad,        // 未找到命令
        _SH_CMD_STATUS_Success,    // 成功找到命令函数
        _SH_CMD_STATUS_Incomplete, // 命令不完整
    } err_status;

    int match_count; // 表示有多少段为成功匹配的命令段

    const sh_cmd_reg_t *reg_struct; // 根命令结构的定义地址

    const sh_cmd_t *match_cmd; // 最后一个匹配的命令信息

} sh_cmd_info_t;

typedef struct // 字符串值解释结果
{
    enum
    {
        _PARSE_TYPE_STRING,   // 字符串
        _PARSE_TYPE_INTEGER,  // 带符号整型
        _PARSE_TYPE_UNSIGNED, // 无符号整型
        _PARSE_TYPE_FLOAT,    // 浮点
    } type;

    union
    {
        const char *val_string; // 指向字符串地址
        int val_integer;        // 带符号整型值
        unsigned val_unsigned;  // 无符号整型值
        float val_float;        // 浮点值
    } value;

    char *endptr; // 指向结束字符

} sh_parse_t;

typedef struct sh_obj_def // 对象内存结构
{
    sh_port_t port;

    sys_psnode_t obj_key_node;
    sh_key_reg_t obj_key_data;

    sys_psnode_t obj_cmd_node;
    sh_cmd_reg_t obj_cmd_data;

    const char *prompt; // 提示符

    sys_pslist_t key_list; // 已注册的热键链
    char key_str[8];       // 已缓存的热键代码
    uint16_t key_stored;   // 已缓存的热键数

    sys_pslist_t cmd_list;                         // 已注册的仅属于对应的终端可见的根命令链
    char cmd_line[CONFIG_SH_MAX_LINE_LEN / 4 * 4]; // 当前命令缓存（包括 模块提示、命令行，不包括 提示符）
    char *cmd_buf;                                 // 当前命令行在 cmd_line[] 中的指针
    uint16_t cmd_stored;                           // 当前已缓存字符数（不包括 提示符 和 模块提示）
    uint16_t cmd_input;                            // 当前光标位置（0.._MAX_CMD_LEN）
    uint16_t sync_cursor;                          // 与当前光标同步的位置
    int cmd_return;                                // 指令执行的结果

    const sh_cmd_reg_t *select_reg_struct; // 内部 select 命令
    const sh_cmd_t *select_cmd;            // 内部 select 命令

    char *cmd_history;          // 命令的历史记录
    int history_size;           // cmd_history 的长度
    uint16_t *history_index;    // 历史记录信息
    uint16_t history_index_num; // history_index 的成员数
    uint16_t history_valid_num; // 已记录的历史命令数
    uint16_t history_show;      // 当前正在显示历史命令。如果在显示历史命令时输入任意有效字符，则 cmd_line[] 将立即被更新

    char *cmd_bank; // 用于保存 sh_ctrl_del_word(), sh_ctrl_del_left(), sh_ctrl_del_right() 删除的内容
    int bank_size;  // cmd_bank 的大小（建议值为 CONFIG_SH_MAX_LINE_LEN ）

    sh_cp_info_t *cp_info;          // 在允许传递 TAB 键到函数并预执行时暂存信息
    int cp_match_num;               // 记录 sh_completion_resource() 需要保持的信息
    int cp_list_algin;              // 记录 sh_completion_resource() 需要保持的信息
    uint8_t cp_resource_flag;       // 标记执行了 sh_completion_resource()
    uint8_t tab_press_count;        // cp_fn() 执行次数计数
    uint8_t exec_flag;              // 禁止重入关键函数
    volatile sh_cp_op_t cp_operate; // 保存最后一次自动补全的执行效果

    bool disable_echo;    // 关闭回显
    bool disable_history; // 关闭记录历史（以节约CPU开销）
} sh_t;

#define _SH_DO_CONCAT(X, Y) X##Y
#define _SH_CONCAT(X, Y) _SH_DO_CONCAT(X, Y)
#define _SH_NAME(NAME) _SH_CONCAT(_SH_CONCAT(_SH_CONCAT(__, NAME), __), __LINE__)
#define _SH_GENERIC_SUB(SUB) (__builtin_types_compatible_p(__typeof(SUB), void *)                        ? _SH_SUB_TYPE_VOID \
                              : __builtin_types_compatible_p(__typeof(SUB), __typeof(_sh_generic_cp_fn)) ? _SH_SUB_TYPE_CPFN \
                              : __builtin_types_compatible_p(__typeof(SUB), sh_cmd_t const[])            ? _SH_SUB_TYPE_SUB  \
                                                                                                         : ~0u)

/* 定义命令 ------------------------------------------------------------------------- */

/**
 * @brief 定义执行命令的函数
 *
 * @param sh_hdl 由 sh_init_vt100() 初始化，在 sh_putc() 传入入的对象
 * @param argc 已输入参数的数量
 * @param argv 已输入参数的指针（字符串）
 *
 * @verbatim
 * @c sh.c
 * @endverbatim
 */
#define SH_CMD_FN(NAME) __used static int NAME(sh_t *sh_hdl, int argc, const char *argv[])

/**
 * @brief 定义执行自动补全的函数，当收到按键为 TAB 时被调用。
 *
 * @param sh_hdl 由 sh_init_vt100() 初始化，在 sh_putc() 传入入的对象
 * @param argc 已输入参数的数量
 * @param argv 已输入参数的指针（字符串）
 * @param flag false 表示最后一个参数正在输入，true 表示最后一个参数已完整输入（光标前一个字符是空格）
 *
 * @verbatim
 * @c sh.c
 * @endverbatim
 */
#define SH_CMD_CP_FN(NAME) __used static void NAME(sh_t *sh_hdl, int argc, const char *argv[], bool flag)

/**
 * @brief 定义命令列表 SH_REGISTER_CMD 或 SH_DEF_SUB_CMD 中的成员
 *
 * @param CMD 命令（字符串，不要有空格）
 * @param HELP 命令的帮助信息（字符串）
 * @param FUNC 执行命令对的函数( static int _cmd_func(sh_t *sh_hdl, int argc, const char *argv[]) )。如果即值为 NULL 时，表示有子命令。
 * @param SUB_NAME
 *   1. 如果 FUNC 取值为 NULL 时， SUB_NAME 必须指向 SH_DEF_SUB_CMD 定义的子命令列表名；
 *   2. 如果 FUNC 取值为函数时:
 *      2.1 SUB_NAME 可为 NULL；
 *      2.1 SUB_NAME 指向 void (*cp_fn)(sh_t *sh_hdl, int argc, const char *argv[]) 表示按 TAB 键时被执行的函数，用于参数补全。
 *
 * @verbatim
 * @c sh.c
 * @endverbatim
 */
#define SH_SETUP_CMD(CMD, HELP, FUNC, SUB) \
    {                                      \
        .cmd = CMD,                        \
        .help = HELP,                      \
        .cmd_func = FUNC,                  \
        .sub_fn = {SUB},                   \
        .fn_type = _SH_GENERIC_SUB(SUB),   \
    }

/**
 * @brief 定义子命令列表（由 SH_SETUP_CMD 中的参数 SUB_NAME 所指向的列表）。
 *
 * @param SUB_NAME 子命令列表名。需要与 SH_SETUP_CMD 中的参数 SUB_NAME 的值保持一致
 *
 * @verbatim
 * @c sh.c
 * @endverbatim
 */
#define SH_DEF_SUB_CMD(SUB_NAME, ...)    \
    static sh_cmd_t const SUB_NAME[] = { \
        __VA_ARGS__{0}}

/**
 * @brief 定义根命令列表。
 *
 * @param NAME 描述自动初始化的函数名。
 *
 * @param ... 使用 SH_SETUP_CMD 添加任意数量的根命令的信息。
 */
#define SH_DEF_CMD(NAME, ...)                       \
    static sys_psnode_t _SH_NAME(cmd_node_);        \
    static sh_cmd_t const _SH_NAME(cmd_data_)[] = { \
        __VA_ARGS__{0}};                            \
    static sh_cmd_reg_t const NAME = {              \
        .node = &_SH_NAME(cmd_node_),               \
        .cmd = _SH_NAME(cmd_data_),                 \
        .file = __FILE__,                           \
        .line = __LINE__,                           \
    }

/**
 * @brief 注册根命令列表。将在初始化阶段被自动执行。
 *
 * @param NAME 描述自动初始化的函数名。
 *
 * @param ... 使用 SH_SETUP_CMD 添加任意数量的根命令的信息。
 *
 * @verbatim
 * @c sh.c
 * @endverbatim
 */
#define SH_REGISTER_CMD(NAME, ...)     \
    SH_DEF_CMD(NAME, __VA_ARGS__);     \
    static int _init_##NAME(void)      \
    {                                  \
        return sh_register_cmd(&NAME); \
    }                                  \
    INIT_EXPORT_COMPONENT(_init_##NAME)

SH_CMD_CP_FN(_sh_generic_cp_fn) {}

/* 注册命令 -------------------------------------------------------------------------- */

int sh_register_cmd(const sh_cmd_reg_t *sh_reg);      // 可用自动初始化宏执行 @ref SH_REGISTER_CMD
int sh_register_cmd_hide(const sh_cmd_reg_t *sh_reg); // 可用自动初始化宏执行 @ref SH_REGISTER_CMD
int sh_unregister_cmd(const sh_cmd_reg_t *sh_reg);    // 取消注册一个根命令

/* 现成终端协议初始化 ----------------------------------------------------------------- */

extern sh_t g_uart_handle_vt100; // 内部默认已初始化的句柄

int sh_init_vt100(sh_t *sh_hdl, sh_vprint_fn vprint, sh_disconnect_fn disconnect); // 初始化一个新的 VT100 的终端对象

void sh_config_history_mem(sh_t *sh_hdl, void *mem, int size); // 选用：配置用于记录历史的缓存
void sh_config_backup_mem(sh_t *sh_hdl, void *mem, int size);  // 选用：配置用于保存 sh_ctrl_del_word(), sh_ctrl_del_left(), sh_ctrl_del_right() 删除的内容

/* 执行过程 --------------------------------------------------------------------------- */

void sh_putc(sh_t *sh_hdl, char c);            // 记录一个字符到内部缓存中并解析和执行
void sh_putstr(sh_t *sh_hdl, const char *str); // 同 sh_putc()

void sh_putstr_quiet(sh_t *sh_hdl, const char *str); // 同 sh_putstr() 但不会回显

void sh_set_prompt(sh_t *sh_hdl, const char *prompt); // 设置提示符

void sh_reset_line(sh_t *sh_hdl); // 清除命令接收缓存

int sh_echo(sh_t *sh_hdl, const char *fmt, ...);  // 回显到终端

sh_parse_t sh_parse_value(const char *str); // 参数工具：解析字符串表示的数值

int sh_merge_param(char *dst, int len, int argc, const char *argv[]); // 参数工具：以空格为分隔符，合并多个参数

int sh_get_cmd_result(sh_t *sh_hdl); // 获取上一条有效命令的返回值

void sh_refresh_line(sh_t *sh_hdl); // 执行刷一次当前命令行显示

/* cp_fn 应用 ---------------------------------------------------------------------- */

sh_cmd_info_t sh_cmd_info(sh_t *sh_hdl, int argc, const char *argv[]); // 根据参数，获取完全匹配的命令信息

sh_cp_op_t sh_completion_cmd(sh_t *sh_hdl, int argc, const char *argv[]); // 根据参数，在已注册命令中查找并自动补全命令

sh_cp_op_t sh_completion_param(sh_t *sh_hdl, const sh_cp_param_t *param); // 仅在 cp_fn() 中可用，根据当前命令行内容，在给出的参数结构中查找并自动补全参数。参数允许为 NULL

void sh_completion_resource(sh_t *sh_hdl, const char *arg_str, const char *res_str, const char *help); // 仅在 cp_fn() 中可用，根据当前命令行内容，逐个给出可供查找的提示符。内部将根据所提供的数据自动补全
sh_cp_op_t sh_get_cp_result(sh_t *sh_hdl);                                                             // 配合 sh_completion_resource() 使用，在下次进入 cp_fn() 时获取前一次自动补全的执行效果

/* 热键功能同步 ----------------------------------------------------------------------- */

int sh_register_port(sh_t *sh_hdl, const sh_port_t *port);           // 配置终端的控制接口
int sh_register_key(sh_t *sh_hdl, const sh_key_reg_t *sh_key);       // 注册一组热键
int sh_unregister_key(sh_t *sh_hdl, const sh_key_reg_t *sh_key);     // 取消注册一组热键
int sh_register_key_cmd(sh_t *sh_hdl, const sh_cmd_reg_t *sh_reg);   // 注册仅属于对应的终端可见的根命令 @ref SH_REGISTER_CMD
int sh_unregister_key_cmd(sh_t *sh_hdl, const sh_cmd_reg_t *sh_reg); // 取消注册仅属于对应的终端可见的根命令

void sh_ctrl_tab(sh_t *sh_hdl);                 // 同步热键功能：自动实例/打印命令
bool sh_ctrl_enter(sh_t *sh_hdl);               // 同步热键功能：执行已缓存到的命令
bool sh_ctrl_delete(sh_t *sh_hdl, int n);       // 同步热键功能：删除光标的后 n 个字符。返回是否成功
bool sh_ctrl_backspace(sh_t *sh_hdl, int n);    // 同步热键功能：删除光标的前 n 个字符。返回是否成功
bool sh_ctrl_up(sh_t *sh_hdl);                  // 同步热键功能：查看上一个命令，并更新光标位置记录。返回是否成功
bool sh_ctrl_down(sh_t *sh_hdl);                // 同步热键功能：查看下一个命令，并更新光标位置记录。返回是否成功
void sh_ctrl_left(sh_t *sh_hdl);                // 同步热键功能：光标左移，并更新光标位置记录
void sh_ctrl_right(sh_t *sh_hdl);               // 同步热键功能：光标右移，并更新光标位置记录
void sh_ctrl_home(sh_t *sh_hdl);                // 同步热键功能：光标移到开头位置，并更新光标位置记录
void sh_ctrl_end(sh_t *sh_hdl);                 // 同步热键功能：光标移到结尾位置，并更新光标位置记录
void sh_ctrl_set_cursor(sh_t *sh_hdl, int pos); // 同步热键功能：光标移到指定位置（不包括提示符，起始位置为0），并更新光标位置记录
void sh_ctrl_word_head(sh_t *sh_hdl);           // 同步热键功能：光标移到当前单词的开头，并更新光标位置记录
void sh_ctrl_word_tail(sh_t *sh_hdl);           // 同步热键功能：光标移到当前单词的结尾，并更新光标位置记录
void sh_ctrl_print_cmd_line(sh_t *sh_hdl);      // 同步热键功能：打印一次命令行内容
unsigned sh_ctrl_del_word(sh_t *sh_hdl);        // 同步热键功能：擦除光标前的单词内容。返回：成功擦除的字符数
unsigned sh_ctrl_del_left(sh_t *sh_hdl);        // 同步热键功能：擦除光标前到行首的全部内容。返回：成功擦除的字符数
unsigned sh_ctrl_del_right(sh_t *sh_hdl);       // 同步热键功能：擦除光标后到行尾的全部内容。返回：成功擦除的字符数
bool sh_ctrl_undelete(sh_t *sh_hdl);            // 同步热键功能：插入 sh_ctrl_del_word(), sh_ctrl_del_left(), sh_ctrl_del_right() 擦除的内容。返回是否成功
bool sh_ctrl_is_module(sh_t *sh_hdl);           // 同步热键功能：查询当前是否使用了 select 命令进入的模块模式（快捷命令模式）

void sh_ctrl_set_input_pos(sh_t *sh_hdl, uint16_t input_pos); // 设置当前指针（光标）位置（不包括提示符和模块提示），起始位置为0
uint16_t sh_ctrl_get_input_pos(sh_t *sh_hdl);                 // 获取当前指针（光标）位置（不包括提示符和模块提示），起始位置为0

uint16_t sh_ctrl_get_line_pos(sh_t *sh_hdl);            // 根据当前显示内容（包含回翻的历史记录），获取当前光标位置（提示符+模块提示+命令）
uint16_t sh_ctrl_get_line_len(sh_t *sh_hdl);            // 根据当前显示内容（包含回翻的历史记录），获取最大光标位置（提示符+模块提示+命令）
char sh_ctrl_get_line_char(sh_t *sh_hdl, uint16_t pos); // 获取命令行部分的字符（提示符+模块提示+命令）

/* 其他 ------------------------------------------------------------------------------- */

void sh_register_external(sh_vprint_fn out); // 用于显式初始化内部函数

#endif
