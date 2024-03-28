/**
 * @file sys_init.h
 * @author LokLiang (lokliang@163.com)
 * @brief
 * @version 0.1
 * @date 2023-06-01
 * @date 2023-05-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __SYS_INIT_H__
#define __SYS_INIT_H__

#include "sys_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* #ifdef __cplusplus */

/* initialize function -------------------------------------------------------- */

#define INIT_EXPORT_BOARD(FN)               _INIT_EXPORT(FN, 0, 1)       // 定义自动初始化函数：非常早期的初始化，此时调度器还未启动
#define INIT_EXPORT_PREV(FN)                _INIT_EXPORT(FN, 1, 1)       // 定义自动初始化函数：主要是用于纯软件的初始化、没有太多依赖的函数
#define INIT_EXPORT_DEVICE(FN)              _INIT_EXPORT(FN, 2, 1)       // 定义自动初始化函数：外设驱动初始化相关，比如网卡设备
#define INIT_EXPORT_COMPONENT(FN)           _INIT_EXPORT(FN, 3, 1)       // 定义自动初始化函数：组件初始化，比如文件系统或者 LWIP
#define INIT_EXPORT_ENV(FN)                 _INIT_EXPORT(FN, 4, 1)       // 定义自动初始化函数：系统环境初始化，比如挂载文件系统
#define INIT_EXPORT_APP(FN, PRIOR)          _INIT_EXPORT(FN, 5, PRIOR)   // 定义自动初始化函数：应用初始化，比如 GUI 应用（PRIOR 建议取值为 0 或 10..99）

/* module define -------------------------------------------------------------- */

#define OBJ_EXPORT_DEVICE(OBJECT, NAME)     _MODULE_EXPORT(OBJECT, NAME, MODULE_TYPE_DEVICE)
#define OBJ_EXPORT_COMPONENT(OBJECT, NAME)  _MODULE_EXPORT(OBJECT, NAME, MODULE_TYPE_COMPONENT)

/* module api------------------------------------------------------------------ */

__static_inline const void *device_binding(const char *name);
__static_inline const char *device_name(const void *object);
__static_inline const char *device_search(const char *prefix_name, unsigned int num);

__static_inline const void *component_binding(const char *name);
__static_inline const char *component_name(const void *object);
__static_inline const char *component_search(const char *prefix_name, unsigned int num);

/* code ----------------------------------------------------------------------- */

/** @defgroup initialize function
 * @{
 */

typedef struct
{
    int (*fn)(void);     // 初始化函数地址
    const char *fn_name; // debug: 函数关联名
    const char *file;    // debug: 所在文件名
    uint16_t line;       // debug: 所在文件行
    uint8_t level;       // debug: 初始化等级
    uint8_t prior;       // debug: 初始化优先级
} sys_init_t;

/**
 * @def _INIT_EXPORT
 *
 * @brief 用于如下宏：
 *          INIT_EXPORT_BOARD
 *          INIT_EXPORT_PREV
 *          INIT_EXPORT_DEVICE
 *          INIT_EXPORT_COMPONENT
 *          INIT_EXPORT_ENV
 *          INIT_EXPORT_APP
 * 输出一个 sys_init_t 类型的数据。
 * 这些数据将被归为 "(.s_sys_init_t.*)" 段中，编译阶段被排序并链接。
 * 需要将文件 "sys_init.ld" 添加到链接脚本。
 * 用于自动初始化。
 *
 * @param FN 待函数
 *
 * @param LEVEL 按功能划分的优先级
 *
 * @param PRIOR 同一功能等级中的优先级，0..99，数值越小越优早执行
 *
 * 举例：
 * @verbatim
 * static int _init_env_template(void)
 * {
 *     SYS_LOG_INF("done.\r\n");
 *     return 0;
 * }
 * INIT_EXPORT_ENV(_init_env_template);
 *
 * static int _init_app_template1(void)
 * {
 *     SYS_LOG_INF("done.\r\n");
 *     return 0;
 * }
 * static int _init_app_template2(void)
 * {
 *     SYS_LOG_INF("done.\r\n");
 *     return 0;
 * }
 * INIT_EXPORT_APP(_init_app_template1, 0);
 * INIT_EXPORT_APP(_init_app_template2, 1);
 * @endverbatim
 */
#define _INIT_EXPORT(FN, LEVEL, PRIOR)                                           \
    __used __section(".s_sys_init_t." #LEVEL "." #PRIOR) static sys_init_t const \
        __sys_init_##FN = {                                                      \
            .fn = FN,                                                            \
            .fn_name = #FN,                                                      \
            .line = __LINE__,                                                    \
            .file = __FILE__,                                                    \
            .level = LEVEL,                                                      \
            .prior = PRIOR,                                                      \
    }

/**
 * @}
 */

/** @defgroup initialize module
 * @{
 */

typedef enum __packed // 定义模块的类型
{
    MODULE_TYPE_DEVICE,    // 模块类型为驱动
    MODULE_TYPE_COMPONENT, // 模块类型为组件
    // ...
} module_type_t;

typedef struct // 模块数据结构
{
    const char *name;   // 模块数据关联名
    const void *obj;    // 模块数据地址
    module_type_t type; // 模块数据类型
    uint16_t line;      // debug: 所在文件行
    const char *file;   // debug: 所在文件名
} module_t;

/**
 * @def _MODULE_EXPORT
 *
 * @brief 输出一个指定模块数据结构。用于如下宏：
 *          OBJ_EXPORT_DEVICE
 *          OBJ_EXPORT_COMPONENT
 * 这些数据结构将被归为 ".sys_module_data" 段中，编译阶段被链接。
 * 需要将文件 "sys_init.ld" 添加到链接脚本。
 * 使用 sys_module_binding() 可找到模块数据结构。
 *
 * @param OBJECT 子对象（符号）
 *
 * @param OBJ_NAME 子对象关联名称（字符串）。使用 sys_module_search() 可遍历已有对象关联名称
 *
 * @param MODULE_TYPE 模块类型 @ref module_type_t
 *
 * 举例：
 * @verbatim
 * #include "device_template.h"
 *
 * static void _set_data(device_template_t dev, int data);
 * static int _get_data(device_template_t dev);
 *
 * static device_template_config_t config;
 *
 * static struct device_template const device = {
 *     .api = {
 *         .set_data = _set_data,
 *         .get_data = _get_data,
 *     },
 *     .config = &config,
 * };
 * OBJ_EXPORT_DEVICE(device, "DRV1");
 *
 * static void _set_data(device_template_t dev, int data)
 * {
 *     dev->config->data = data;
 * }
 *
 * static int _get_data(device_template_t dev)
 * {
 *     return dev->config->data;
 * }
 * @endverbatim
 *
 */
#define _MODULE_EXPORT(OBJECT, OBJ_NAME, MODULE_TYPE)             \
    __used __section(".sys_module_data.1") static module_t const \
        __sys_module_##OBJECT = {                                \
            .name = OBJ_NAME,                                    \
            .obj = &OBJECT,                                      \
            .type = MODULE_TYPE,                                 \
            .line = __LINE__,                                    \
            .file = __FILE__,                                    \
    }

/**
 * @brief 结合类型和名称，精确匹配查找个由 _MODULE_EXPORT 输出的模块对象。
 *
 * @param type 由 _MODULE_EXPORT 输出的模块类型
 * @param name 由 _MODULE_EXPORT 输出的模块名
 * @return module_t * 由 _MODULE_EXPORT 输出的模块对象
 */
__static_inline const module_t *sys_module_binding(module_type_t type, const char *name)
{
    if (name)
    {
        extern const module_t __sys_module_leader_0;
        extern const module_t __sys_module_leader_9;
        const module_t *module = &__sys_module_leader_0;
        const module_t *end = &__sys_module_leader_9;
        while (module < end)
        {
            if (module->type == type)
            {
                int i;
                for (i = 0; 1; i++)
                {
                    if (name[i] != module->name[i])
                        break;
                    if (name[i] == '\0')
                        return module;
                }
            }
            module = &module[1];
        }
    }
    return NULL;
}

/**
 * @brief 结合类型和名称，查找个由 _MODULE_EXPORT 输出的模块对象名。
 *
 * @param type 由 _MODULE_EXPORT 输出的模块类型
 * @param prefix_name 由 _MODULE_EXPORT 输出的模块名的前缀。例如： prefix_name 为 "UART" ，可能查找到 "UART1", "UART2"...
 * @param num 匹配到 prefix_name 的第几次时返回，范围 0..n
 * @return const char * 匹配到的第 num 个名称为 prefix_name.* 的完整对象名，这个对象名可用于 sys_module_binding() 的参数。
 * @return NULL 表示没对象
 */
__static_inline const char *sys_module_search(module_type_t type, const char *prefix_name, unsigned int num)
{
    if (prefix_name)
    {
        extern const module_t __sys_module_leader_0;
        extern const module_t __sys_module_leader_9;
        const module_t *module = &__sys_module_leader_0;
        const module_t *end = &__sys_module_leader_9;
        unsigned int match = 0;
        while (module < end)
        {
            if (module->type == type)
            {
                int i;
                for (i = 0; 1; i++)
                {
                    if (prefix_name[i] == '\0')
                    {
                        if (match < num)
                        {
                            match++;
                            break;
                        }
                        else
                        {
                            return module->name;
                        }
                    }
                    if (prefix_name[i] != module->name[i])
                        break;
                }
            }
            module = &module[1];
        }
    }
    return NULL;
}

/**
 * @brief 获取已绑定的对象的关联名
 *
 * @param object 由 sys_module_binding() 取得的对象
 * @return const char * 对象的关联名
 */
__static_inline const char *sys_module_name(const void *object)
{
    if (object)
    {
        extern const module_t __sys_module_leader_0;
        extern const module_t __sys_module_leader_9;
        const module_t *module = &__sys_module_leader_0;
        const module_t *end = &__sys_module_leader_9;
        while (module < end)
        {
            if (module->obj == object)
            {
                return module->name;
            }
            module = &module[1];
        }
    }
    return "";
}

/**
 * @}
 */

/** @defgroup module
 * @{
 */

/**
 * @brief 绑定一个由 OBJ_EXPORT_DEVICE 输出的设备对象
 *
 * @param name 由 OBJ_EXPORT_DEVICE 输出的设备名
 * @return const void * 由 OBJ_EXPORT_DEVICE 输出的设备对象
 */
__static_inline const void *device_binding(const char *name)
{
    const module_t *module = sys_module_binding(MODULE_TYPE_DEVICE, name);
    if (module)
        return module->obj;
    else
        return NULL;
}

/**
 * @brief 获取已绑定的对象的关联名
 *
 * @param object 由 device_binding() 取得的对象
 * @return const char * 对象的关联名
 */
__static_inline const char *device_name(const void *object)
{
    return sys_module_name(object);
}

/**
 * @brief 查找 device_binding() 可用的模块对象名。
 *
 * @param prefix_name 由 _MODULE_EXPORT 输出的模块名的前缀。例如： prefix_name 为 "UART" ，可能查找到 "UART1", "UART2"...
 * @param num 匹配到 prefix_name 的第几次时返回，范围 0..n
 * @return const char * 匹配到的第 num 个名称为 prefix_name.* 的完整对象名
 * @return NULL 表示没对象
 */
__static_inline const char *device_search(const char *prefix_name, unsigned int num)
{
    return sys_module_search(MODULE_TYPE_DEVICE, prefix_name, num);
}

/**
 * @brief 绑定一个由 OBJ_EXPORT_COMPONENT 输出的设备对象
 *
 * @param name 由 OBJ_EXPORT_COMPONENT 输出的设备名
 * @return const void * 由 OBJ_EXPORT_COMPONENT 输出的设备对象
 */
__static_inline const void *component_binding(const char *name)
{
    const module_t *module = sys_module_binding(MODULE_TYPE_COMPONENT, name);
    if (module)
        return module->obj;
    else
        return NULL;
}


/**
 * @brief 获取已绑定的对象的关联名
 *
 * @param object 由 component_binding() 取得的对象
 * @return const char * 对象的关联名
 */
__static_inline const char *component_name(const void *object)
{
    return sys_module_name(object);
}

/**
 * @brief 查找 component_binding() 可用的模块对象名。
 *
 * @param prefix_name 由 _MODULE_EXPORT 输出的模块名的前缀。例如： prefix_name 为 "UART" ，可能查找到 "UART1", "UART2"...
 * @param num 匹配到 prefix_name 的第几次时返回，范围 0..n
 * @return const char * 匹配到的第 num 个名称为 prefix_name.* 的完整对象名
 * @return NULL 表示没对象
 */
__static_inline const char *component_search(const char *prefix_name, unsigned int num)
{
    return sys_module_search(MODULE_TYPE_COMPONENT, prefix_name, num);
}

/**
 * @}
 */

#ifdef __cplusplus
} /* extern "C" */
#endif /* #ifdef __cplusplus */

#endif
