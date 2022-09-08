#include "jr.h"
#include "jr_node.h"
#include "jr_parser.h"
#include "jr_type.h"
/* meld-cut-here */
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Source: https://stackoverflow.com/a/18298965
#ifndef thread_local
#if __STDC_VERSION__ >= 201112 && !defined __STDC_NO_THREADS__
#define thread_local _Thread_local
#elif defined _WIN32 && (defined _MSC_VER || defined __ICL ||                  \
                         defined __DMC__ || defined __BORLANDC__)
#define thread_local __declspec(thread)
/* note that ICC (linux) and Clang are covered by __GNUC__ */
#elif defined __GNUC__ || defined __SUNPRO_C || defined __xlC__
#define thread_local __thread
#else
#error "Cannot define thread_local"
#endif
#endif

static thread_local int error = JR_OK;

enum offset
{
    PARSER_OFFSET = 0,
    CURSOR_OFFSET = 1,
    NODE_OFFSET = 2,
};

static inline struct jr_parser *get_parser(struct jr jr[])
{
    return &jr[PARSER_OFFSET].parser;
}
static inline struct jr_cursor *cursor(struct jr jr[])
{
    return &jr[CURSOR_OFFSET].cursor;
}
static inline struct jr_node *nodes(struct jr jr[])
{
    return &jr[NODE_OFFSET].node;
}
static inline struct jr_node *cnode(struct jr jr[])
{
    return nodes(jr) + cursor(jr)->pos;
}
static inline struct jr_node *sentinel(struct jr jr[])
{
    return &nodes(jr)[get_parser(jr)->size];
}
static inline void delimit(struct jr jr[])
{
    cursor(jr)->json[cnode(jr)->end] = '\0';
}
static inline void input_errno(void)
{
    if (errno == EINVAL) error = JR_INVAL;
    if (errno == ERANGE) error = JR_OUTRANGE;
}
static inline char *cstring(struct jr jr[])
{
    return &cursor(jr)->json[cnode(jr)->start];
}
static inline char *empty_string(struct jr jr[])
{
    return &cursor(jr)->json[cursor(jr)->length];
}
static void sentinel_init(struct jr jr[]);
static long strto_long(const char *restrict, char **restrict, int);
static unsigned long strto_ulong(const char *restrict, char **restrict, int);
static double strto_double(const char *restrict, char **restrict);
static int jr_strlcpy(char *dst, const char *src, int size);
extern void jr_parser_init(struct jr_parser *parser, int size);
extern void jr_parser_reset(struct jr_parser *parser);
extern int jr_parser_parse(struct jr_parser *, int length, char *json,
                           int nnodes, struct jr_node *);
extern void jr_cursor_init(struct jr_cursor *cursor, int length, char *json);

void __jr_init(struct jr jr[], int alloc_size)
{
    error = JR_OK;
    jr_parser_init(get_parser(jr), alloc_size);
}

int jr_parse(struct jr jr[], int length, char *json)
{
    error = JR_OK;
    jr_parser_reset(get_parser(jr));
    jr_cursor_init(cursor(jr), length, json);
    struct jr_parser *p = get_parser(jr);
    struct jr_cursor *c = cursor(jr);
    int n = p->alloc_size - 2;
    int rc = jr_parser_parse(p, c->length, c->json, n, nodes(jr));
    if (rc) return rc;
    sentinel_init(jr);
    if (p->size > 0) cnode(jr)->parent = -1;
    return rc;
}

int jr_error(void) { return error; }

void jr_reset(struct jr jr[])
{
    error = JR_OK;
    cursor(jr)->pos = 0;
    for (int i = 0; i <= get_parser(jr)->size; ++i)
        nodes(jr)[i].prev = 0;
}

int jr_type(struct jr const jr[])
{
    return nodes((struct jr *)jr)[cursor((struct jr *)jr)->pos].type;
}

int jr_nchild(struct jr const jr[]) { return cnode((struct jr *)jr)->size; }

struct jr *jr_back(struct jr jr[])
{
    cursor(jr)->pos = cnode(jr)->prev;
    return jr;
}

static struct jr *setup_sentinel(struct jr jr[])
{
    nodes(jr)[get_parser(jr)->size].prev = cursor(jr)->pos;
    cursor(jr)->pos = get_parser(jr)->size;
    return jr;
}

struct jr *jr_down(struct jr jr[])
{
    if (jr_type(jr) == JR_SENTINEL) return jr;

    if (cnode(jr)->size == 0) return setup_sentinel(jr);

    return jr_next(jr);
}

struct jr *jr_next(struct jr jr[])
{
    if (jr_type(jr) == JR_SENTINEL) return jr;

    if (cursor(jr)->pos + 1 >= get_parser(jr)->size) return setup_sentinel(jr);

    nodes(jr)[cursor(jr)->pos + 1].prev = cursor(jr)->pos;
    cursor(jr)->pos++;
    return jr;
}

static struct jr *rollback(struct jr jr[], int pos)
{
    while (cursor(jr_back(jr))->pos != pos)
        ;
    return jr;
}

struct jr *jr_right(struct jr jr[])
{
    if (jr_type(jr) == JR_SENTINEL) return jr;

    int parent = cnode(jr)->parent;
    int pos = cursor(jr)->pos;
    if (parent == -1) return setup_sentinel(jr);
    while (parent != cnode(jr_next(jr))->parent)
    {
        if (jr_type(jr) == JR_SENTINEL)
        {
            setup_sentinel(rollback(jr, pos));
            break;
        }
    }
    return jr;
}

struct jr *jr_up(struct jr jr[])
{
    if (jr_type(jr) == JR_SENTINEL) return jr;

    int parent = cnode(jr)->parent;
    if (parent == -1) return setup_sentinel(jr);

    nodes(jr)[parent].prev = cursor(jr)->pos;
    cursor(jr)->pos = parent;
    return jr;
}

struct jr *jr_array_at(struct jr jr[], int idx)
{
    if (jr_type(jr) != JR_ARRAY)
    {
        error = JR_INVAL;
        return jr;
    }

    int pos = cursor(jr)->pos;
    jr_down(jr);
    for (int i = 0; i < idx; ++i)
    {
        if (jr_type(jr) == JR_SENTINEL) break;
        jr_right(jr);
    }
    if (jr_type(jr) == JR_SENTINEL)
    {
        rollback(jr, pos);
        error = JR_OUTRANGE;
    }
    return jr;
}

struct jr *jr_object_at(struct jr jr[], char const *key)
{
    if (jr_type(jr) != JR_OBJECT)
    {
        error = JR_INVAL;
        return jr;
    }

    int pos = cursor(jr)->pos;
    jr_down(jr);
    while (strcmp(jr_as_string(jr), key))
    {
        if (jr_type(jr) == JR_SENTINEL)
        {
            rollback(jr, pos);
            error = JR_NOTFOUND;
            return jr;
        }
        jr_right(jr);
    }
    return jr_down(jr);
}

char *jr_string_of(struct jr jr[], char const *key)
{
    if (jr_type(jr) != JR_OBJECT) error = JR_INVAL;
    if (error) return empty_string(jr);

    int pos = cursor(jr)->pos;
    jr_object_at(jr, key);
    char *str = jr_as_string(jr);
    rollback(jr, pos);
    return str;
}

void jr_strcpy_of(struct jr jr[], char const *key, char *dst, int size)
{
    if (size > 0) dst[0] = '\0';

    if (jr_type(jr) != JR_OBJECT) error = JR_INVAL;
    if (error) return;

    int pos = cursor(jr)->pos;
    jr_object_at(jr, key);
    char *str = jr_as_string(jr);
    if (jr_strlcpy(dst, str, size) >= size) error = JR_NOMEM;
    rollback(jr, pos);
}

long jr_long_of(struct jr jr[], char const *key)
{
    if (jr_type(jr) != JR_OBJECT) error = JR_INVAL;
    if (error) return 0;

    int pos = cursor(jr)->pos;
    jr_object_at(jr, key);
    long val = jr_as_long(jr);
    rollback(jr, pos);
    return val;
}

unsigned long jr_ulong_of(struct jr jr[], char const *key)
{
    if (jr_type(jr) != JR_OBJECT) error = JR_INVAL;
    if (error) return 0;

    int pos = cursor(jr)->pos;
    jr_object_at(jr, key);
    unsigned long val = jr_as_ulong(jr);
    rollback(jr, pos);
    return val;
}

char *jr_as_string(struct jr jr[])
{
    if (jr_type(jr) != JR_STRING) error = JR_INVAL;
    if (error) return empty_string(jr);

    delimit(jr);
    return cstring(jr);
}

bool jr_as_bool(struct jr jr[])
{
    if (jr_type(jr) != JR_BOOL) error = JR_INVAL;
    if (error) return false;

    return cstring(jr)[0] == 't';
}

void *jr_as_null(struct jr jr[])
{
    if (jr_type(jr) != JR_NULL) error = JR_INVAL;
    return NULL;
}

long jr_as_long(struct jr jr[])
{
    if (jr_type(jr) != JR_NUMBER) error = JR_INVAL;
    if (error) return 0;

    delimit(jr);
    long val = strto_long(cstring(jr), NULL, 10);
    input_errno();
    return val;
}

unsigned long jr_as_ulong(struct jr jr[])
{
    if (jr_type(jr) != JR_NUMBER) error = JR_INVAL;
    if (error) return 0;

    delimit(jr);
    unsigned long val = strto_ulong(cstring(jr), NULL, 10);
    input_errno();
    return val;
}

double jr_as_double(struct jr jr[])
{
    if (jr_type(jr) != JR_NUMBER) error = JR_INVAL;
    if (error) return 0;

    delimit(jr);
    double val = strto_double(cstring(jr), NULL);
    input_errno();
    return val;
}

static void sentinel_init(struct jr jr[])
{
    sentinel(jr)->type = JR_SENTINEL;
    sentinel(jr)->start = 0;
    sentinel(jr)->end = 1;
    sentinel(jr)->size = 0;
    sentinel(jr)->parent = get_parser(jr)->size;
    sentinel(jr)->prev = get_parser(jr)->size;
}

static long strto_long(const char *restrict nptr, char **restrict endptr,
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

static unsigned long strto_ulong(const char *restrict nptr,
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

static double strto_double(const char *restrict nptr, char **restrict endptr)
{
    errno = 0;
    return strtod(nptr, endptr);
}

static int jr_strlcpy(char *dst, const char *src, int size)
{
    int ret = (int)strlen(src);

    if (size > 0)
    {
        int len = (ret >= size) ? size - 1 : ret;
        memcpy(dst, src, len);
        dst[len] = '\0';
    }
    return ret;
}
/* meld-cut-here */
