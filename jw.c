#include "jw.h"

/* meld-cut-here */
static unsigned put_unquoted_cstr(char buf[], char const *str);
static unsigned itoa_rev(char buf[], long i);
static void reverse(char buf[], int size);

unsigned jw_bool(char buf[], bool x)
{
    return put_unquoted_cstr(buf, x ? "true" : "false");
}

unsigned jw_int(char buf[], long x)
{
    unsigned size = itoa_rev(buf, x);
    reverse(buf, size);
    return size;
}

unsigned jw_null(char buf[]) { return put_unquoted_cstr(buf, "null"); }

unsigned jw_str(char buf[], char const x[])
{
    buf[0] = '\"';
    unsigned size = put_unquoted_cstr(buf + 1, x);
    buf[size + 1] = '\"';
    return size + 2;
}

unsigned jw_object_open(char buf[])
{
    buf[0] = '{';
    return 1;
}

unsigned jw_object_close(char buf[])
{
    buf[0] = '}';
    return 1;
}

unsigned jw_array_open(char buf[])
{
    buf[0] = '[';
    return 1;
}

unsigned jw_array_close(char buf[])
{
    buf[0] = ']';
    return 1;
}

unsigned jw_comma(char buf[])
{
    buf[0] = ',';
    return 1;
}

unsigned jw_colon(char buf[])
{
    buf[0] = ':';
    return 1;
}

static unsigned put_unquoted_cstr(char buf[], char const *cstr)
{
    char *p = buf;
    while (*cstr)
        *p++ = *cstr++;
    return (unsigned)(p - buf);
}

static unsigned itoa_rev(char buf[], long i)
{
    char *dst = buf;
    if (i == 0)
    {
        *dst++ = '0';
    }
    int neg = (i < 0) ? -1 : 1;
    while (i)
    {
        *dst++ = (char)('0' + (neg * (i % 10)));
        i /= 10;
    }
    if (neg == -1)
    {
        *dst++ = '-';
    }
    return (unsigned)(dst - buf);
}

#define XOR_SWAP(a, b)                                                         \
    do                                                                         \
    {                                                                          \
        a ^= b;                                                                \
        b ^= a;                                                                \
        a ^= b;                                                                \
    } while (0)

static void reverse(char buf[], int size)
{
    char *end = buf + size - 1;

    while (buf < end)
    {
        XOR_SWAP(*buf, *end);
        buf++;
        end--;
    }
}
/* meld-cut-here */
