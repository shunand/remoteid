#ifndef __MISC_H__
#define __MISC_H__

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief 让 CPU 空转延时
 * 
 * @param us 微秒
 */
void drv_misc_busy_wait(unsigned us);

/**
 * @brief 获取变量的位带映射地址
 *
 * @param mem 变量地址
 * @return 位带映射地址。如无返回 NULL
 */
unsigned *drv_misc_bitband(void *mem);

/**
 * @brief 读取 MCU 身份信息的 MD5 值
 * 
 * @param out[out] 输出
 * @retval 0 成功
 * @retval -1 失败
 */
int drv_misc_read_id_md5(unsigned char out[16]);

/**
 * @brief 设置中断向量地址
 *
 * @param vector 中断向量地址
 */
void drv_misc_set_vector(void *vector);

/**
 * @brief 获取中断向量地址
 *
 * @return void* 中断向量地址
 */
void *drv_misc_get_vector(void);

/**
 * @brief 根据具体平台，以尽可能快的速度把内存置0
 * 
 * @param src 内存地址
 * @param len 内存长度
 */
void mem_reset(void *src, unsigned len);

/**
 * @brief 根据具体平台，以尽可能快的速度复制内存数据
 * 
 * @param dest[out] 目录内存
 * @param src 源内存
 * @param len 长度
 */
void mem_cpy(void *dest, const void *src, unsigned len);

#ifdef __cplusplus
}
#endif

#endif
