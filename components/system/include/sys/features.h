/**
 * @file features.h
 * @author 
 * @brief 
 * In order to redefine int32_t to int when compiles using GCC, this file is
 * added to override <sys/features.h> of GCC.
 * @version 0.1
 * @date 2021-12-09
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef _LIBC_SYS_FEATURES_H_
#define _LIBC_SYS_FEATURES_H_

/*
 * Redefine some macros defined by GCC
 */
#ifdef __INT32_MAX__
#undef __INT32_MAX__
#endif
#define __INT32_MAX__ 2147483647

#ifdef __UINT32_MAX__
#undef __UINT32_MAX__
#endif
#define __UINT32_MAX__ 4294967295U

#ifdef __INT_LEAST32_MAX__
#undef __INT_LEAST32_MAX__
#endif
#define __INT_LEAST32_MAX__ 2147483647

#ifdef __UINT_LEAST32_MAX__
#undef __UINT_LEAST32_MAX__
#endif
#define __UINT_LEAST32_MAX__ 4294967295U

#ifdef __INT_LEAST32_TYPE__
#undef __INT_LEAST32_TYPE__
#endif
#define __INT_LEAST32_TYPE__ int

#ifdef __UINT_LEAST32_TYPE__
#undef __UINT_LEAST32_TYPE__
#endif
#define __UINT_LEAST32_TYPE__ unsigned int

#ifdef __INT32_C
#undef __INT32_C
#endif
#define __INT32_C(c) c

#ifdef __UINT32_C
#undef __UINT32_C
#endif
#define __UINT32_C(c) c ## U

#ifdef __INT32_TYPE__
#undef __INT32_TYPE__
#endif
#define __INT32_TYPE__ int

#ifdef __UINT32_TYPE__
#undef __UINT32_TYPE__
#endif
#define __UINT32_TYPE__ unsigned int

/*
 * Include <sys/features.h> of compiler
 */
#include_next <sys/features.h>

#endif /* _LIBC_SYS_FEATURES_H_ */
