[简要](../../../README.md)

# shell

  > 是一款适合单片机用的简单的 shell 模块，使单片也能像 Linux 一样，具体基本的命令行功能，适用于示例、调试、应用等场合。

## 快捷键支持
  - Tap：       补全命令或参数
  - Up：        查看上一个命令
  - Down：      查看下一个命令
  - Home：      光标移到开头位置
  - End：       光标移到结尾位置
  - Ctrl+C:     结束程序
  - Ctrl+D:     删除光标后一个字符或结束程序
  - Ctrl+L:     清空终端
  - Ctrl+A:     同 Home
  - Ctrl+E:     同 End
  - Ctrl+U:     擦除光标前到行首的全部内容
  - Ctrl+K:     擦除光标后到行尾的全部内容
  - Ctrl+W:     擦除光标前的单词
  - Ctrl+Y:     还原擦除的内容
  - Ctrl+Right: 光标移动到单词首位
  - Ctrl+Left:  光标移动到单词结尾

## 如何使用

### 移植
  > 调用 sh_init_vt100() 进行初始化时指定并实现两个对应的接口即可，实际上在 [sh_vt100.c](./sh_vt100.c) 已做好一系列的工作，一般情况下无需再另外实现，除非有多数据接口的需求。

### 初始化
  > 在 [sh_t100.c](./sh_vt100.c) 中已通过 _install_shell_vt100() 被初始化，对象 g_uart_handle_vt100 直接可用。

### 注册命令列表
  > 所有命令都必须先注册才可生效。

  - 使用宏 SH_REGISTER_CMD 注册
    > 使用自动初始化的宏注册一个父命令列表。以下摘自 [sc.h](./sh.c) 文件的命令定义部分。
```c
SH_DEF_SUB_CMD(
    _cmd_echo_sublist,
    SH_SETUP_CMD("on", "Enable to feedback the command line", _cmd_echo_on, NULL),    //
    SH_SETUP_CMD("off", "Disable to feedback the command line", _cmd_echo_off, NULL), //
);

SH_DEF_SUB_CMD(
    _cmd_sh_sublist,
    SH_SETUP_CMD("echo", "Turn on/off echo through sh_echo()", NULL, _cmd_echo_sublist),                                         //
    SH_SETUP_CMD("history", "Show history control", _cmd_history, _cmd_history_param),                                           //
    SH_SETUP_CMD("list-init", "List all auto initialize function\r\n\t* Usage: list-init [filter]", _cmd_print_init_fn, NULL),   //
    SH_SETUP_CMD("list-module", "List all module structure\r\n\t* Usage: list-module [filter]", _cmd_print_module_struct, NULL), //
    SH_SETUP_CMD("version", "Print the shell version", _cmd_version, NULL),                                                      //
    SH_SETUP_CMD("parse-value", "Parse the value of a string demo", _cmd_parse_test, NULL),                                      //
);

SH_REGISTER_CMD(
    register_internal_command,
    SH_SETUP_CMD("sh", "Internal command", NULL, _cmd_sh_sublist),                                   //
    SH_SETUP_CMD("help", "Print the complete root command", _cmd_print_help, _cmd_print_help_param), //
    SH_SETUP_CMD("select", "Select parent command", _cmd_select, _cmd_select_param),                 //
    SH_SETUP_CMD("exit", "Exit parent command or disconnect", _cmd_exit, NULL),                      //
);
```

  - 使用函数接口动态注册
    - sh_register_cmd(const sh_cmd_reg_t *sh_reg);
    - sh_register_cmd_hide(const sh_cmd_reg_t *sh_reg);
    - sh_unregister_cmd(const sh_cmd_reg_t *sh_reg);

### 设置子命令、命令勾子和补全勾子
  >  命令勾子和执行补全功能的勾子函数都在命令列表中确定，它们分别位于 SH_SETUP_CMD 的第 3、4 个参数。

  - 命令勾子
    > 如实例中显示，位于 SH_SETUP_CMD 的第 3 个参数，设置命令勾子。  
    > 如果命令勾子取值为函数地址，则表示这个命令为完整命令，此时第 4 个参数必须是 cp_fn ，并且可设置对补全勾子或者为 NULL；  
    > 如果命令勾子取值为NULL，则表示这个命令为父命令，此时第 4 个参数必须是 sub_cmd ，并且指向另一个由 SH_DEF_SUB_CMD 定义的命令列表；  

### 命令勾子的可用接口
  - sh_set_prompt()
  - sh_reset_line()
  - sh_echo()
  - sh_parse_value()
  - sh_merge_param()

### 补全勾子的可用接口
  - sh_putstr_quiet()
  - sh_cmd_info()
  - sh_completion_cmd()
  - sh_completion_param()
  - sh_completion_resource()
  - sh_get_cp_result()

### 数据流向
  - 输入
    > 任何需要解析的数据由 sh_putc() 或 sh_putstr() 开始, 在内部最终完成解析和回调执行的整个过程；

  - 输出
    > 命令勾子或补全勾子可使用如下函数回显：
      - sh_echo()
      - sh_set_prompt()
      - sh_reset_line()

## 参考实例
  - [heap_shell.c](../../../architecture/common/heap/heap_shell.c)
  - [fatfs_shell.c](../../../source/components/fs/fatfs_shell.c)
