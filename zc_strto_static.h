#ifndef ZC_STRTO_STATIC_H
#define ZC_STRTO_STATIC_H

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>

#define ZC_API static

ZC_API long long zc_strto_llong(const char *restrict nptr,
                                char **restrict endptr, int base)
{
    errno = 0;
    intmax_t v = strtoimax(nptr, endptr, base);
    if (errno)
    {
        if (v == INTMAX_MAX) return LLONG_MAX;
        if (v == INTMAX_MIN) return LLONG_MIN;
        assert(v == 0);
        return 0;
    }
    if (v > LLONG_MAX)
    {
        errno = ERANGE;
        return LLONG_MAX;
    }
    if (v < LLONG_MIN)
    {
        errno = ERANGE;
        return LLONG_MIN;
    }
    return (long long)v;
}

ZC_API long zc_strto_long(const char *restrict nptr, char **restrict endptr,
                          int base)
{
    errno = 0;
    intmax_t v = strtoimax(nptr, endptr, base);
    if (errno)
    {
        if (v == INTMAX_MAX) return LONG_MAX;
        if (v == INTMAX_MIN) return LONG_MIN;
        assert(v == 0);
        return 0;
    }
    if (v > LONG_MAX)
    {
        errno = ERANGE;
        return LONG_MAX;
    }
    if (v < LONG_MIN)
    {
        errno = ERANGE;
        return LONG_MIN;
    }
    return (long)v;
}

ZC_API int zc_strto_int(const char *restrict nptr, char **restrict endptr,
                        int base)
{
    errno = 0;
    intmax_t v = strtoimax(nptr, endptr, base);
    if (errno)
    {
        if (v == INTMAX_MAX) return INT_MAX;
        if (v == INTMAX_MIN) return INT_MIN;
        assert(v == 0);
        return 0;
    }
    if (v > INT_MAX)
    {
        errno = ERANGE;
        return INT_MAX;
    }
    if (v < INT_MIN)
    {
        errno = ERANGE;
        return INT_MIN;
    }
    return (int)v;
}

ZC_API short zc_strto_short(const char *restrict nptr, char **restrict endptr,
                            int base)
{
    errno = 0;
    intmax_t v = strtoimax(nptr, endptr, base);
    if (errno)
    {
        if (v == INTMAX_MAX) return SHRT_MAX;
        if (v == INTMAX_MIN) return SHRT_MIN;
        assert(v == 0);
        return 0;
    }
    if (v > SHRT_MAX)
    {
        errno = ERANGE;
        return SHRT_MAX;
    }
    if (v < SHRT_MIN)
    {
        errno = ERANGE;
        return SHRT_MIN;
    }
    return (short)v;
}

ZC_API unsigned long long zc_strto_ullong(const char *restrict nptr,
                                          char **restrict endptr, int base)
{
    errno = 0;
    uintmax_t v = strtoumax(nptr, endptr, base);
    if (errno)
    {
        if (v == UINTMAX_MAX) return ULLONG_MAX;
        assert(v == 0);
        return 0;
    }
    if (v > ULLONG_MAX)
    {
        errno = ERANGE;
        return ULLONG_MAX;
    }
    return (unsigned long long)v;
}

ZC_API unsigned long zc_strto_ulong(const char *restrict nptr,
                                    char **restrict endptr, int base)
{
    errno = 0;
    uintmax_t v = strtoumax(nptr, endptr, base);
    if (errno)
    {
        if (v == UINTMAX_MAX) return ULONG_MAX;
        assert(v == 0);
        return 0;
    }
    if (v > ULONG_MAX)
    {
        errno = ERANGE;
        return ULONG_MAX;
    }
    return (unsigned long)v;
}

ZC_API unsigned int zc_strto_uint(const char *restrict nptr,
                                  char **restrict endptr, int base)
{
    errno = 0;
    uintmax_t v = strtoumax(nptr, endptr, base);
    if (errno)
    {
        if (v == UINTMAX_MAX) return UINT_MAX;
        assert(v == 0);
        return 0;
    }
    if (v > UINT_MAX)
    {
        errno = ERANGE;
        return UINT_MAX;
    }
    return (unsigned int)v;
}

ZC_API unsigned short zc_strto_ushort(const char *restrict nptr,
                                      char **restrict endptr, int base)
{
    errno = 0;
    uintmax_t v = strtoumax(nptr, endptr, base);
    if (errno)
    {
        if (v == UINTMAX_MAX) return USHRT_MAX;
        assert(v == 0);
        return 0;
    }
    if (v > USHRT_MAX)
    {
        errno = ERANGE;
        return USHRT_MAX;
    }
    return (unsigned short)v;
}

ZC_API int64_t zc_strto_int64(const char *restrict nptr, char **restrict endptr,
                              int base)
{
    errno = 0;
    intmax_t v = strtoimax(nptr, endptr, base);
    if (errno)
    {
        if (v == INTMAX_MAX) return INT64_MAX;
        if (v == INTMAX_MIN) return INT64_MIN;
        assert(v == 0);
        return 0;
    }
    if (v > INT64_MAX)
    {
        errno = ERANGE;
        return INT64_MAX;
    }
    if (v < INT64_MIN)
    {
        errno = ERANGE;
        return INT64_MIN;
    }
    return (int64_t)v;
}

ZC_API int32_t zc_strto_int32(const char *restrict nptr, char **restrict endptr,
                              int base)
{
    errno = 0;
    intmax_t v = strtoimax(nptr, endptr, base);
    if (errno)
    {
        if (v == INTMAX_MAX) return INT32_MAX;
        if (v == INTMAX_MIN) return INT32_MIN;
        assert(v == 0);
        return 0;
    }
    if (v > INT32_MAX)
    {
        errno = ERANGE;
        return INT32_MAX;
    }
    if (v < INT32_MIN)
    {
        errno = ERANGE;
        return INT32_MIN;
    }
    return (int32_t)v;
}

ZC_API int16_t zc_strto_int16(const char *restrict nptr, char **restrict endptr,
                              int base)
{
    errno = 0;
    intmax_t v = strtoimax(nptr, endptr, base);
    if (errno)
    {
        if (v == INTMAX_MAX) return INT16_MAX;
        if (v == INTMAX_MIN) return INT16_MIN;
        assert(v == 0);
        return 0;
    }
    if (v > INT16_MAX)
    {
        errno = ERANGE;
        return INT16_MAX;
    }
    if (v < INT16_MIN)
    {
        errno = ERANGE;
        return INT16_MIN;
    }
    return (int16_t)v;
}

ZC_API int8_t zc_strto_int8(const char *restrict nptr, char **restrict endptr,
                            int base)
{
    errno = 0;
    intmax_t v = strtoimax(nptr, endptr, base);
    if (errno)
    {
        if (v == INTMAX_MAX) return INT8_MAX;
        if (v == INTMAX_MIN) return INT8_MIN;
        assert(v == 0);
        return 0;
    }
    if (v > INT8_MAX)
    {
        errno = ERANGE;
        return INT8_MAX;
    }
    if (v < INT8_MIN)
    {
        errno = ERANGE;
        return INT8_MIN;
    }
    return (int8_t)v;
}

ZC_API uint64_t zc_strto_uint64(const char *restrict nptr,
                                char **restrict endptr, int base)
{
    errno = 0;
    uintmax_t v = strtoumax(nptr, endptr, base);
    if (errno)
    {
        if (v == UINTMAX_MAX) return UINT64_MAX;
        assert(v == 0);
        return 0;
    }
    if (v > UINT64_MAX)
    {
        errno = ERANGE;
        return UINT64_MAX;
    }
    return (uint64_t)v;
}

ZC_API uint32_t zc_strto_uint32(const char *restrict nptr,
                                char **restrict endptr, int base)
{
    errno = 0;
    uintmax_t v = strtoumax(nptr, endptr, base);
    if (errno)
    {
        if (v == UINTMAX_MAX) return UINT32_MAX;
        assert(v == 0);
        return 0;
    }
    if (v > UINT32_MAX)
    {
        errno = ERANGE;
        return UINT32_MAX;
    }
    return (uint32_t)v;
}

ZC_API uint16_t zc_strto_uint16(const char *restrict nptr,
                                char **restrict endptr, int base)
{
    errno = 0;
    uintmax_t v = strtoumax(nptr, endptr, base);
    if (errno)
    {
        if (v == UINTMAX_MAX) return UINT16_MAX;
        assert(v == 0);
        return 0;
    }
    if (v > UINT16_MAX)
    {
        errno = ERANGE;
        return UINT16_MAX;
    }
    return (uint16_t)v;
}

ZC_API uint8_t zc_strto_uint8(const char *restrict nptr, char **restrict endptr,
                              int base)
{
    errno = 0;
    uintmax_t v = strtoumax(nptr, endptr, base);
    if (errno)
    {
        if (v == UINTMAX_MAX) return UINT8_MAX;
        assert(v == 0);
        return 0;
    }
    if (v > UINT8_MAX)
    {
        errno = ERANGE;
        return UINT8_MAX;
    }
    return (uint8_t)v;
}

ZC_API float zc_strto_float(const char *restrict nptr, char **restrict endptr)
{
    errno = 0;
    return strtof(nptr, endptr);
}

ZC_API double zc_strto_double(const char *restrict nptr, char **restrict endptr)
{
    errno = 0;
    return strtod(nptr, endptr);
}

ZC_API long double zc_strto_ldouble(const char *restrict nptr,
                                    char **restrict endptr)
{
    errno = 0;
    return strtold(nptr, endptr);
}

#endif
