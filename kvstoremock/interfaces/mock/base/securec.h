/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SECUREC_H__5D13A042_DC3F_4ED9_A8D1_882811274C27
#define __SECUREC_H__5D13A042_DC3F_4ED9_A8D1_882811274C27

/* Compile in kernel under macro control */
#ifndef SECUREC_IN_KERNEL
#ifdef __KERNEL__
#define SECUREC_IN_KERNEL 1
#else
#define SECUREC_IN_KERNEL 0
#endif
#endif

/* If you need high performance, enable the SECUREC_WITH_PERFORMANCE_ADDONS macro, default is enable .
 * The macro is automatically closed on the windows platform in securectyp.h.
 */
#ifndef SECUREC_WITH_PERFORMANCE_ADDONS
#if SECUREC_IN_KERNEL
#define SECUREC_WITH_PERFORMANCE_ADDONS 0
#else
#define SECUREC_WITH_PERFORMANCE_ADDONS 1
#endif
#endif

#include <stdarg.h>
#include "securectype.h"

#ifndef SECUREC_HAVE_ERRNO_H
#if SECUREC_IN_KERNEL
#define SECUREC_HAVE_ERRNO_H 0
#else
#define SECUREC_HAVE_ERRNO_H 1
#endif
#endif

/* EINVAL ERANGE may defined in errno.h */
#if SECUREC_HAVE_ERRNO_H
#include <errno.h>
#endif

/* If stack size on some embedded platform is limited, you can define the following macro
 * which will put some variables on heap instead of stack.
 * SECUREC_STACK_SIZE_LESS_THAN_1K
 */

/* define error code */
#if !defined(__STDC_WANT_LIB_EXT1__) || (defined(__STDC_WANT_LIB_EXT1__) && (__STDC_WANT_LIB_EXT1__ == 0))
#ifndef SECUREC_DEFINED_ERRNO_TYPE
#define SECUREC_DEFINED_ERRNO_TYPE
/* just check whether macrodefinition exists. */
#ifndef errno_t
typedef int errno_t;
#endif
#endif
#endif

/* success */
#ifndef EOK
#define EOK (0)
#endif

#ifndef EINVAL
/* The src buffer is not correct and destination buffer can't not be reset */
#define EINVAL (22)
#endif

#ifndef EINVAL_AND_RESET
/* Once the error is detected, the dest buffer must be rest! */
#define EINVAL_AND_RESET (22 | 128)
#endif

#ifndef ERANGE
/* The destination buffer is not long enough and destination buffer can not be reset */
#define ERANGE (34)
#endif

#ifndef ERANGE_AND_RESET
/* Once the error is detected, the dest buffer must be rest! */
#define ERANGE_AND_RESET  (34 | 128)
#endif

#ifndef EOVERLAP_AND_RESET
/* Once the buffer overlap is detected, the dest buffer must be rest! */
#define EOVERLAP_AND_RESET (54 | 128)
#endif

/* if you need export the function of this library in Win32 dll, use __declspec(dllexport) */
#ifdef SECUREC_IS_DLL_LIBRARY
#ifdef SECUREC_DLL_EXPORT
#define SECUREC_API __declspec(dllexport)
#else
#define SECUREC_API __declspec(dllimport)
#endif
#else
/* Standardized function declaration . If a security function is declared in the your code,
 * it may cause a compilation alarm,Please delete the security function you declared
 */
#define SECUREC_API extern
#endif

#ifndef SECUREC_SNPRINTF_TRUNCATED
#define SECUREC_SNPRINTF_TRUNCATED 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

    SECUREC_API errno_t memset_s(void *dest, size_t destMax, int c, size_t count);

#ifndef SECUREC_ONLY_DECLARE_MEMSET

#if SECUREC_IN_KERNEL == 0
    SECUREC_API errno_t wmemcpy_s(wchar_t *dest, size_t destMax, const wchar_t *src, size_t count);
#endif
    SECUREC_API errno_t memmove_s(void *dest, size_t destMax, const void *src, size_t count);
#if SECUREC_IN_KERNEL == 0
    SECUREC_API errno_t wmemmove_s(wchar_t *dest, size_t destMax, const wchar_t *src, size_t count);

    SECUREC_API errno_t wcscpy_s(wchar_t *strDest, size_t destMax, const wchar_t *strSrc);

    SECUREC_API errno_t wcsncpy_s(wchar_t *strDest, size_t destMax, const wchar_t *strSrc, size_t count);

    SECUREC_API errno_t wcscat_s(wchar_t *strDest, size_t destMax, const wchar_t *strSrc);

    SECUREC_API errno_t wcsncat_s(wchar_t *strDest, size_t destMax, const wchar_t *strSrc, size_t count);

    SECUREC_API char *strtok_s(char *strToken, const char *strDelimit, char **context);

    SECUREC_API wchar_t *wcstok_s(wchar_t *strToken, const wchar_t *strDelimit, wchar_t **context);

    SECUREC_API int sprintf_s(char *strDest, size_t destMax, const char *format, ...) SECUREC_ATTRIBUTE(3, 4);

    SECUREC_API int swprintf_s(wchar_t *strDest, size_t destMax, const wchar_t *format, ...);

    SECUREC_API int vsprintf_s(char *strDest, size_t destMax, const char *format,
                               va_list arglist) SECUREC_ATTRIBUTE(3, 0);

    SECUREC_API int vswprintf_s(wchar_t *strDest, size_t destMax, const wchar_t *format, va_list arglist);

    SECUREC_API int vsnprintf_s(char *strDest, size_t destMax, size_t count, const char *format,
                                va_list arglist) SECUREC_ATTRIBUTE(4, 0);

    SECUREC_API int snprintf_s(char *strDest, size_t destMax, size_t count, const char *format,
                               ...) SECUREC_ATTRIBUTE(4, 5);

#if SECUREC_SNPRINTF_TRUNCATED
    SECUREC_API int vsnprintf_truncated_s(char *strDest, size_t destMax, const char *format,
                                          va_list arglist) SECUREC_ATTRIBUTE(3, 0);

    SECUREC_API int snprintf_truncated_s(char *strDest, size_t destMax,
                                         const char *format, ...) SECUREC_ATTRIBUTE(3, 4);
#endif

    SECUREC_API int scanf_s(const char *format, ...);

    SECUREC_API int wscanf_s(const wchar_t *format, ...);

    SECUREC_API int vscanf_s(const char *format, va_list arglist);

    SECUREC_API int vwscanf_s(const wchar_t *format, va_list arglist);

    SECUREC_API int fscanf_s(FILE *stream, const char *format, ...);

    SECUREC_API int fwscanf_s(FILE *stream, const wchar_t *format, ...);

    SECUREC_API int vfscanf_s(FILE *stream, const char *format, va_list arglist);

    SECUREC_API int vfwscanf_s(FILE *stream, const wchar_t *format, va_list arglist);

    SECUREC_API int sscanf_s(const char *buffer, const char *format, ...);

    SECUREC_API int swscanf_s(const wchar_t *buffer, const wchar_t *format, ...);

    SECUREC_API int vsscanf_s(const char *buffer, const char *format, va_list arglist);

    SECUREC_API int vswscanf_s(const wchar_t *buffer, const wchar_t *format, va_list arglist);

    SECUREC_API char *gets_s(char *buffer, size_t destMax);
#endif

    SECUREC_API errno_t memcpy_s(void *dest, size_t destMax, const void *src, size_t count);

    SECUREC_API errno_t strcpy_s(char *strDest, size_t destMax, const char *strSrc);

    SECUREC_API errno_t strncpy_s(char *strDest, size_t destMax, const char *strSrc, size_t count);

    SECUREC_API errno_t strcat_s(char *strDest, size_t destMax, const char *strSrc);

    SECUREC_API errno_t strncat_s(char *strDest, size_t destMax, const char *strSrc, size_t count);
#if SECUREC_IN_KERNEL == 0
    /* those functions are used by macro ,must declare hare , also for  without function declaration warning */
    extern errno_t strncpy_error(char *strDest, size_t destMax, const char *strSrc, size_t count);
    extern errno_t strcpy_error(char *strDest, size_t destMax, const char *strSrc);
#endif
#endif

#if SECUREC_WITH_PERFORMANCE_ADDONS
    /* those functions are used by macro */
    extern errno_t memset_sOptAsm(void *dest, size_t destMax, int c, size_t count);
    extern errno_t memset_sOptTc(void *dest, size_t destMax, int c, size_t count);
    extern errno_t memcpy_sOptAsm(void *dest, size_t destMax, const void *src, size_t count);
    extern errno_t memcpy_sOptTc(void *dest, size_t destMax, const void *src, size_t count);

/* strcpy_sp is a macro, NOT a function in performance optimization mode. */
#define strcpy_sp(dest, destMax, src)  ((__builtin_constant_p((destMax)) && \
                                         __builtin_constant_p((src))) ?  \
                                        SECUREC_STRCPY_SM((dest), (destMax), (src)) : \
                                        strcpy_s((dest), (destMax), (src)))

/* strncpy_sp is a macro, NOT a function in performance optimization mode. */
#define strncpy_sp(dest, destMax, src, count)  ((__builtin_constant_p((count)) && \
                                                 __builtin_constant_p((destMax)) && \
                                                 __builtin_constant_p((src))) ?  \
                                                SECUREC_STRNCPY_SM((dest), (destMax), (src), (count)) : \
                                                strncpy_s((dest), (destMax), (src), (count)))

/* strcat_sp is a macro, NOT a function in performance optimization mode. */
#define strcat_sp(dest, destMax, src) ((__builtin_constant_p((destMax)) && \
                                        __builtin_constant_p((src))) ?  \
                                       SECUREC_STRCAT_SM((dest), (destMax), (src)) : \
                                       strcat_s((dest), (destMax), (src)))

 /* strncat_sp is a macro, NOT a function in performance optimization mode. */
#define strncat_sp(dest, destMax, src, count) ((__builtin_constant_p((count)) &&  \
                                                __builtin_constant_p((destMax)) && \
                                                __builtin_constant_p((src))) ?  \
                                               SECUREC_STRNCAT_SM((dest), (destMax), (src), (count)) : \
                                               strncat_s((dest), (destMax), (src), (count)))

/* memcpy_sp is a macro, NOT a function in performance optimization mode. */
#define memcpy_sp(dest, destMax, src, count)  (__builtin_constant_p((count)) ? \
                                               (SECUREC_MEMCPY_SM((dest), (destMax),  (src), (count))) : \
                                               (__builtin_constant_p((destMax)) ? \
                                                (((size_t)(destMax) > 0 && \
                                                 (((unsigned long long)(destMax) & \
                                                   (unsigned long long)(-2)) < SECUREC_MEM_MAX_LEN)) ? \
                                                  memcpy_sOptTc((dest), (destMax), (src), (count)) : ERANGE ) : \
                                                  memcpy_sOptAsm((dest), (destMax), (src), (count))))

/* memset_sp is a macro, NOT a function in performance optimization mode. */
#define memset_sp(dest, destMax, c, count)  (__builtin_constant_p((count)) ? \
                                             (SECUREC_MEMSET_SM((dest), (destMax),  (c), (count))) : \
                                             (__builtin_constant_p((destMax)) ? \
                                              (((size_t)(destMax) > 0 && \
                                              (((unsigned long long)(destMax) & \
                                               (unsigned long long)(-2)) < SECUREC_MEM_MAX_LEN)) ? \
                                                memset_sOptTc((dest), (destMax), (c), (count)) : ERANGE ) : \
                                                memset_sOptAsm((dest), (destMax), (c), (count))))
#else
#define strcpy_sp   strcpy_s
#define strncpy_sp  strncpy_s
#define strcat_sp   strcat_s
#define strncat_sp  strncat_s
#define memcpy_sp   memcpy_s
#define memset_sp   memset_s
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __SECUREC_H__5D13A042_DC3F_4ED9_A8D1_882811274C27 */
