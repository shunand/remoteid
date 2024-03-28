#include "drivers/data_port/sb_data_port.h"

#include "os/os.h"

#define CONFIG_SYS_LOG_LEVEL SYS_LOG_LEVEL_INF
#define SYS_LOG_DOMAIN "DATA"
#define CONS_ABORT()
#include "sys_log.h"

/**
 * @brief 由具体驱动实现的，以达到节省资源为主要目的，启动数据接口
 *
 * @param port 由对应的驱动提供的绑定接口获得的句柄
 * @retval 0 成功
 */
int sb_data_port_start(sb_data_port_t *port)
{
    if (sb_data_port_is_started(port))
    {
        SYS_LOG_WRN("Data interface repeat started");
        return 0;
    }
    if (port == NULL || port->vtable == NULL)
    {
        return false;
    }

    if (port->vtable->start)
    {
        return port->vtable->start(port);
    }
    else
    {
        SYS_LOG_ERR("The data interface has not been defined");
        return -1;
    }
}

/**
 * @brief 由具体驱动实现的，以达到节省资源为主要目的，关闭数据接口
 *
 * @param port 由对应的驱动提供的绑定接口获得的句柄
 * @retval 0 成功
 */
int sb_data_port_stop(sb_data_port_t *port)
{
    if (!sb_data_port_is_started(port))
    {
        SYS_LOG_WRN("The data interface has not been started yet");
        return 0;
    }

    if (port->vtable->stop)
    {
        return port->vtable->stop(port);
    }
    else
    {
        SYS_LOG_ERR("The data interface has not been defined");
        return -1;
    }
}

/**
 * @brief 由具体驱动实现的，写数据到对应的接口。
 * 建议这些数据将写入到驱动的缓存中，如果 wait_ms 为 0 立即返回，
 * 如果 wait_ms > 0 则这个时间由本函数计时并处理，但期间可能出现堵塞。
 * 注意如果有选择 wait_ms > 0 的情况，驱动中收取缓存的线程与应用不能是同一线程。
 *
 * @param port      由对应的驱动提供的绑定接口获得的句柄
 * @param data      来源数据
 * @param size      数据长度（字节）
 * @param wait_ms   超时时间（毫秒）
 * @retval < 0 操作失败或在指定的时间内未满足指定的操作长度
 * @retval >= 0 实际已缓存的长度
 */
int sb_data_port_write(sb_data_port_t *port, const void *data, uint32_t size, uint32_t wait_ms)
{
    if (!sb_data_port_is_started(port))
    {
        SYS_LOG_WRN("The data interface has not been started yet");
        return -1;
    }

    if (port->vtable->write)
    {
        int ret = 0;
        int cmp_time = os_get_sys_time();
        while (size)
        {
            int curr_time = os_get_sys_time();
            int wsize = port->vtable->write(port, data, size);
            if (wsize > 0)
            {
                cmp_time = curr_time;
                data = &((uint8_t *)data)[wsize];
                size -= wsize;
                ret += wsize;
            }
            else if (wsize == 0)
            {
                if (wait_ms == 0)
                {
                    break;
                }
                else if (curr_time - cmp_time > wait_ms)
                {
                    return -1;
                }
                else
                {
                    os_thread_sleep(1);
                    continue;
                }
            }
            else
            {
                return -1;
            }
        }

        return ret;
    }
    else
    {
        SYS_LOG_ERR("The data interface has not been defined");
        return -1;
    }
}

/**
 * @brief 由具体驱动实现的，从数据接口中读取已缓存的数据。
 * 如果 wait_ms 为 0 将立即返回，
 * 如果 wait_ms > 0 则这个时间由本函数计时并处理，期间可能出现堵塞。
 * 注意如果有选择 wait_ms > 0 的情况，驱动中收取缓存的线程与应用不能是同一线程。
 *
 * @param port          由对应的驱动提供的绑定接口获得的句柄
 * @param buffer[out]   保存到目标内存。值为 NULL 表示仅释放空间
 * @param length        目标内存长度（字节）
 * @param wait_ms       超时时间（毫秒）
 * @retval < 0 操作失败或在指定的时间内未满足指定的操作长度
 * @retval >= 0 实际已读取的长度
 */
int sb_data_port_read(sb_data_port_t *port, void *buffer, uint32_t length, uint32_t wait_ms)
{
    if (!sb_data_port_is_started(port))
    {
        SYS_LOG_WRN("The data interface has not been started yet");
        return -1;
    }

    if (port->vtable->read)
    {
        int ret = 0;
        int cmp_time = os_get_sys_time();
        while (length)
        {
            int curr_time = os_get_sys_time();
            int rsize = port->vtable->read(port, buffer, length);
            if (rsize > 0)
            {
                cmp_time = curr_time;
                buffer = &((uint8_t *)buffer)[rsize];
                length -= rsize;
                ret += rsize;
            }
            else if (rsize == 0)
            {
                if (wait_ms == 0)
                {
                    break;
                }
                else if (curr_time - cmp_time > wait_ms)
                {
                    return -1;
                }
                else
                {
                    os_thread_sleep(1);
                    continue;
                }
            }
            else
            {
                return -1;
            }
        }

        return ret;
    }
    else
    {
        SYS_LOG_ERR("The data interface has not been defined");
        return -1;
    }
}

/**
 * @brief 由具体驱动实现的，获取当前数据接口是否可用（是否已启动）。
 * 本套接口内部的所有其他接口内部都会优先经过这个接口确认状态，驱动内部的其他接口不需要二次确认。
 *
 * @param port 由对应的驱动提供的绑定接口获得的句柄
 * @retval true 接口已启动并可用
 * @retval false 接口未启动，不可用
 */
bool sb_data_port_is_started(sb_data_port_t *port)
{
    SYS_ASSERT(port, "Data interface not bound");
    SYS_ASSERT(port->vtable, "Data interface not bound");

    if (port == NULL || port->vtable == NULL)
    {
        return false;
    }

    if (port->vtable->is_started)
    {
        return port->vtable->is_started(port);
    }
    else
    {
        SYS_LOG_ERR("The data interface has not been defined");
        return false;
    }
}

/**
 * @brief 由具体驱动实现的，获取当前数据接口的本次可读长度
 *
 * @param port 由对应的驱动提供的绑定接口获得的句柄
 * @retval uint32_t sb_data_port_read() 当前可读取的接收缓存长度
 */
uint32_t sb_data_port_get_rx_length(sb_data_port_t *port)
{
    if (!sb_data_port_is_started(port))
    {
        return 0;
    }

    if (port->vtable->get_rx_length)
    {
        return port->vtable->get_rx_length(port);
    }
    else
    {
        SYS_LOG_ERR("The data interface has not been defined");
        return -1;
    }
}
