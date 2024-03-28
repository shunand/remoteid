/**
 * @file sys_types.h
 * @author LokLiang (lokliang@163.com)
 * @brief
 * @version 0.1
 * @date 2023-05-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __SYS_TYPES_H__
#define __SYS_TYPES_H__

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif /* #ifdef __cplusplus */

#ifndef NULL
#define NULL ((void *)0)
#endif

#if defined(__CC_ARM)

#ifndef __static_inline
#define __static_inline  static __inline
#endif

#ifndef __section
#define __section(X)     __attribute__((section(X)))
#endif

#ifndef __used
#define __used           __attribute__((used))
#endif

#ifndef __weak
#define __weak           __attribute__((weak))
#endif

#ifndef __naked
#define __naked          __attribute__((naked))
#endif

#ifndef __packed
#define __packed         __attribute__((packed))
#endif

#ifndef __aligned
#define __aligned(X)     __attribute__((aligned(X)))
#endif

#ifndef __likely
#define __likely(X)      __builtin_expect(!!(X), 1)
#endif

#ifndef __unlikely
#define __unlikely(X)    __builtin_expect(!!(X), 0)
#endif

#ifndef FALLTHROUGH
#define FALLTHROUGH
#endif

#ifndef __nop
#define __nop()          do { __asm { nop; } } while (0)
#endif

#ifndef __no_optimize
#define __no_optimize
#endif

#ifndef __clz
#define __clz(VAL)       __builtin_clz(VAL);
#endif

#ifndef __typeof
#define __typeof(VAR)    __typeof__((VAR))
#endif

#ifndef __optimize
#define __optimize(X)    __attribute__((optimize("O"#X)))
#endif

#elif defined(__GNUC__) /* #if defined(__CC_ARM) */

#ifndef __static_inline
#define __static_inline  static inline
#endif

#ifndef __section
#define __section(X)     __attribute__((section(X)))
#endif

#ifndef __used
#define __used           __attribute__((used))
#endif

#ifndef __weak
#define __weak           __attribute__((weak))
#endif

#ifndef __naked
#define __naked          __attribute__((naked))
#endif

#ifndef __packed
#define __packed         __attribute__((__packed__))
#endif

#ifndef __aligned
#define __aligned(X)     __attribute__((__aligned__(X)))
#endif

#ifndef __likely
#define __likely(X)      __builtin_expect(!!(X), 1)
#endif

#ifndef __unlikely
#define __unlikely(X)    __builtin_expect(!!(X), 0)
#endif

#ifndef FALLTHROUGH
#define FALLTHROUGH      __attribute__((fallthrough))
#endif

#ifndef __nop
#define __nop()          __asm volatile("nop")
#endif

#ifndef __no_optimize
#define __no_optimize    __attribute__((optimize("O0")))
#endif

#ifndef __clz
#define __clz(VAL)       ((VAL) == 0 ? 32 : __builtin_clz(VAL))
#endif

#ifndef __typeof
#define __typeof(VAR)    __typeof__((VAR))
#endif

#ifndef __optimize
#define __optimize(X)    __attribute__((optimize("O"#X)))
#endif

#else /* #elif defined(__GNUC__) */

#define __static_inline static inline

#define __no_init

#define __nop() __nop

#define __no_optimize

#define __clz(VAL) __builtin_clz(VAL);

#endif /* #elif defined(__GNUC__) */

#ifndef __ARRAY_SIZE
#define __ARRAY_SIZE(ARRAY) (sizeof(ARRAY) / sizeof((ARRAY)[0]))
#endif

#ifndef __no_init
#define __no_init        __section(".noinitialized")
#endif

#ifndef __ram_text
#define __ram_text       __section(".ram_text")
#endif

#ifndef __IM
#define __IM     volatile const      /*! Defines 'read only' structure member permissions */
#endif
#ifndef __OM
#define __OM     volatile            /*! Defines 'write only' structure member permissions */
#endif
#ifndef __IOM
#define __IOM    volatile            /*! Defines 'read / write' structure member permissions */
#endif

#ifndef sys_le16_to_cpu
#define _b_swap_16(X) ((u16_t)((((X) >> 8) & 0xff) | (((X)&0xff) << 8)))

#define _b_swap_32(X) ((u32_t)((((X) >> 24) & 0xff) |   \
                                (((X) >> 8) & 0xff00) | \
                                (((X)&0xff00) << 8) |   \
                                (((X)&0xff) << 24)))

#define _b_swap_64(X) ((u64_t)((((X) >> 56) & 0xff) |       \
                                (((X) >> 40) & 0xff00) |    \
                                (((X) >> 24) & 0xff0000) |  \
                                (((X) >> 8) & 0xff000000) | \
                                (((X)&0xff000000) << 8) |   \
                                (((X)&0xff0000) << 24) |    \
                                (((X)&0xff00) << 40) |      \
                                (((X)&0xff) << 56)))

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define sys_le16_to_cpu(VAL) (VAL)
#define sys_le32_to_cpu(VAL) (VAL)
#define sys_le64_to_cpu(VAL) (VAL)
#define sys_cpu_to_le16(VAL) (VAL)
#define sys_cpu_to_le32(VAL) (VAL)
#define sys_cpu_to_le64(VAL) (VAL)
#define sys_be16_to_cpu(VAL) _b_swap_16(VAL)
#define sys_be32_to_cpu(VAL) _b_swap_32(VAL)
#define sys_be64_to_cpu(VAL) _b_swap_64(VAL)
#define sys_cpu_to_be16(VAL) _b_swap_16(VAL)
#define sys_cpu_to_be32(VAL) _b_swap_32(VAL)
#define sys_cpu_to_be64(VAL) _b_swap_64(VAL)
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define sys_le16_to_cpu(VAL) _b_swap_16(VAL)
#define sys_le32_to_cpu(VAL) _b_swap_32(VAL)
#define sys_le64_to_cpu(VAL) _b_swap_64(VAL)
#define sys_cpu_to_le16(VAL) _b_swap_16(VAL)
#define sys_cpu_to_le32(VAL) _b_swap_32(VAL)
#define sys_cpu_to_le64(VAL) _b_swap_64(VAL)
#define sys_be16_to_cpu(VAL) (VAL)
#define sys_be32_to_cpu(VAL) (VAL)
#define sys_be64_to_cpu(VAL) (VAL)
#define sys_cpu_to_be16(VAL) (VAL)
#define sys_cpu_to_be32(VAL) (VAL)
#define sys_cpu_to_be64(VAL) (VAL)
#else
#error "Unknown byte order"
#endif
#endif

typedef uint8_t               u8_t   ;
typedef uint16_t              u16_t  ;
typedef uint32_t              u32_t  ;
typedef int8_t                s8_t   ;
typedef int16_t               s16_t  ;
typedef int32_t               s32_t  ;
typedef int64_t               s64_t  ;
typedef volatile uint8_t      vu8_t  ;
typedef volatile uint16_t     vu16_t ;
typedef volatile uint32_t     vu32_t ;
typedef volatile uint64_t     vu64_t ;
typedef volatile int8_t       vs8_t  ;
typedef volatile int16_t      vs16_t ;
typedef volatile int32_t      vs32_t ;
typedef volatile int64_t      vs64_t ;
typedef volatile signed       vint   ;
typedef unsigned              uint   ;
typedef volatile unsigned     vuint  ;

typedef enum
{
    TYPES_NO_ERR = 0,
    TYPES_TIMEOUT,
    TYPES_NO_MEM,
    TYPES_HAL_ERR,
    TYPES_ERROR = -1,
} types_err_t;

typedef enum
{
    TYPES_RESET = 0,
    TYPES_SET = 1,
} types_flag_t;

typedef enum
{
    TYPES_DISABLE = 0,
    TYPES_ENABLE = 1,
} types_ctrl_t;

#ifndef __WRITEBITS
#define __WRITEBITS(P, INDEX, WIDE, VALUE) (*(vuint *)(P) = (*(vuint *)(P) & ~(~(-1u << (WIDE)) << (INDEX))) ^ (((VALUE) & ~(-1u << (WIDE))) << (INDEX)))
#endif

#ifndef __READBITS
#define __READBITS(VAR, INDEX, WIDE) (((VAR) >> (INDEX)) & ~(-1u << (WIDE)))
#endif

__static_inline void writebits(volatile void *dest, u8_t index, u8_t bitwide, uint value)
{
    while(index + bitwide > sizeof(vuint) * 8);
    while(value >= 1 << bitwide);
    __WRITEBITS(dest, index, bitwide, value);
}

__static_inline uint readbits(uint var, u8_t index, u8_t bitwide)
{
    return __READBITS(var, index, bitwide);
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* #ifdef __cplusplus */

#endif
