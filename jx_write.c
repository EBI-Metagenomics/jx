#include "jx_write.h"

static unsigned put_unquoted_cstr(char buf[], char const *str);
static unsigned itoa_rev(char buf[], long i);
static void reverse(char buf[], int size);

unsigned jx_put_bool(char buf[], bool val)
{
    return put_unquoted_cstr(buf, val ? "true" : "false");
}

unsigned jx_put_int(char buf[], long val)
{
    unsigned size = itoa_rev(buf, val);
    reverse(buf, size);
    return size;
}

unsigned jx_put_null(char buf[]) { return put_unquoted_cstr(buf, "null"); }

unsigned jx_put_str(char buf[], char const val[])
{
    buf[0] = '\"';
    unsigned size = put_unquoted_cstr(buf + 1, val);
    buf[size + 1] = '\"';
    return size + 2;
}

unsigned jx_put_object_open(char buf[])
{
    buf[0] = '{';
    return 1;
}

unsigned jx_put_object_close(char buf[])
{
    buf[0] = '}';
    return 1;
}

unsigned jx_put_array_open(char buf[])
{
    buf[0] = '[';
    return 1;
}

unsigned jx_put_array_close(char buf[])
{
    buf[0] = ']';
    return 1;
}

unsigned jx_put_comma(char buf[])
{
    buf[0] = ',';
    return 1;
}

unsigned jx_put_colon(char buf[])
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

// swap the values in the two given variables
// XXX: fails when a and b refer to same memory location
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
