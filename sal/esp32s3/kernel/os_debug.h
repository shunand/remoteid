#ifndef _OS_DEBUG_H_
#define _OS_DEBUG_H_

#undef CONFIG_SYS_LOG_LEVEL
#define CONFIG_SYS_LOG_LEVEL SYS_LOG_LEVEL_INF
#define SYS_LOG_DOMAIN "OS"
#include "sys_log.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define OS_LOG(FLAG, FMT, ...) _DO_SYS_LOG(FLAG, FMT, ##__VA_ARGS__)
#define OS_DBG(FMT, ...) SYS_LOG_DBG(FMT, ##__VA_ARGS__)
#define OS_INF(FMT, ...) SYS_LOG_INF(FMT, ##__VA_ARGS__)
#define OS_WRN(FMT, ...) SYS_LOG_WRN(FMT, ##__VA_ARGS__)
#define OS_ERR(FMT, ...) SYS_LOG_ERR(FMT, ##__VA_ARGS__)

#define OS_ASS_ISR() SYS_ASSERT(!os_is_isr_context(), "function '%s' exec in ISR contex", __FUNCTION__)
#define OS_ASS_HDL(EXP, HANDLE) SYS_ASSERT(EXP, "handle %p", HANDLE)

#ifdef __cplusplus
}
#endif

#endif /* _OS_DEBUG_H_ */
