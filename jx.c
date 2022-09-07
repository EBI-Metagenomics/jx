#include "jx.h"
#include "jx_compiler.h"
#include "jx_node.h"
#include "jx_parser.h"
#include "jx_type.h"
#include "zc_strto_static.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>

static thread_local int error = 0;

enum offset
{
    PARSER_OFFSET = 0,
    CURSOR_OFFSET = 1,
    NODE_OFFSET = 2,
};

static inline struct jx_parser *get_parser(struct jx jx[])
{
    return &jx[PARSER_OFFSET].parser;
}
static inline struct jx_cursor *cursor(struct jx jx[])
{
    return &jx[CURSOR_OFFSET].cursor;
}
static inline struct jx_node *nodes(struct jx jx[])
{
    return &jx[NODE_OFFSET].node;
}
static inline struct jx_node *cnode(struct jx jx[])
{
    return nodes(jx) + cursor(jx)->pos;
}
static inline struct jx_node *sentinel(struct jx jx[])
{
    return &nodes(jx)[get_parser(jx)->count];
}
static inline void delimit(struct jx jx[])
{
    cursor(jx)->json[cnode(jx)->end] = '\0';
}
static inline void input_errno(void)
{
    if (errno == EINVAL) error = JX_INVAL;
    if (errno == ERANGE) error = JX_OUTRANGE;
}
static inline char *cstring(struct jx jx[])
{
    return &cursor(jx)->json[cnode(jx)->start];
}
static inline char *empty_string(struct jx jx[])
{
    return &cursor(jx)->json[cursor(jx)->length];
}
static void sentinel_init(struct jx jx[]);

void __jx_init(struct jx jx[], int size)
{
    __jx_parser_init(get_parser(jx), size);
    cursor(jx)->pos = 0;
}

int jx_parse(struct jx jx[], char *json)
{
    __jx_cursor_init(cursor(jx), json);
    struct jx_parser *parser = get_parser(jx);
    int rc = __jx_parser_parse(parser, cursor(jx)->length, cursor(jx)->json,
                               parser->alloc_size, nodes(jx));
    if (rc) return rc;
    sentinel_init(jx);
    if (parser->count > 0) cnode(jx)->parent = -1;
    return rc;
}

int jx_error(void) { return error; }

void jx_reset(struct jx jx[])
{
    error = JX_OK;
    cursor(jx)->pos = 0;
    for (int i = 0; i <= get_parser(jx)->count; ++i)
        nodes(jx)[i].prev = 0;
}

int jx_type(struct jx const jx[])
{
    return nodes((struct jx *)jx)[cursor((struct jx *)jx)->pos].type;
}

struct jx *jx_back(struct jx jx[])
{
    cursor(jx)->pos = cnode(jx)->prev;
    return jx;
}

static struct jx *setup_sentinel(struct jx jx[])
{
    nodes(jx)[get_parser(jx)->count].prev = cursor(jx)->pos;
    cursor(jx)->pos = get_parser(jx)->count;
    return jx;
}

struct jx *jx_down(struct jx jx[])
{
    if (jx_type(jx) == JX_SENTINEL) return jx;

    if (cnode(jx)->size == 0) return setup_sentinel(jx);

    return jx_next(jx);
}

struct jx *jx_next(struct jx jx[])
{
    if (jx_type(jx) == JX_SENTINEL) return jx;

    if (cursor(jx)->pos + 1 >= get_parser(jx)->count) return setup_sentinel(jx);

    nodes(jx)[cursor(jx)->pos + 1].prev = cursor(jx)->pos;
    cursor(jx)->pos++;
    return jx;
}

static struct jx *rollback(struct jx jx[], int pos)
{
    while (cursor(jx_back(jx))->pos != pos)
        ;
    return jx;
}

struct jx *jx_right(struct jx jx[])
{
    if (jx_type(jx) == JX_SENTINEL) return jx;

    int parent = cnode(jx)->parent;
    int pos = cursor(jx)->pos;
    if (parent == -1) return setup_sentinel(jx);
    while (parent != cnode(jx_next(jx))->parent)
    {
        if (jx_type(jx) == JX_SENTINEL)
        {
            setup_sentinel(rollback(jx, pos));
            break;
        }
    }
    return jx;
}

struct jx *jx_up(struct jx jx[])
{
    if (jx_type(jx) == JX_SENTINEL) return jx;

    int parent = cnode(jx)->parent;
    if (parent == -1) return setup_sentinel(jx);

    nodes(jx)[parent].prev = cursor(jx)->pos;
    cursor(jx)->pos = parent;
    return jx;
}

struct jx *jx_array_at(struct jx jx[], int idx)
{
    if (jx_type(jx) != JX_ARRAY)
    {
        error = JX_INVAL;
        return jx;
    }

    int pos = cursor(jx)->pos;
    jx_down(jx);
    for (int i = 0; i < idx; ++i)
    {
        if (jx_type(jx) == JX_SENTINEL) break;
        jx_right(jx);
    }
    if (jx_type(jx) == JX_SENTINEL)
    {
        rollback(jx, pos);
        error = JX_OUTRANGE;
    }
    return jx;
}

struct jx *jx_object_at(struct jx jx[], char const *key)
{
    if (jx_type(jx) != JX_OBJECT)
    {
        error = JX_INVAL;
        return jx;
    }

    int pos = cursor(jx)->pos;
    jx_down(jx);
    while (strcmp(jx_as_string(jx), key))
    {
        if (jx_type(jx) == JX_SENTINEL)
        {
            rollback(jx, pos);
            error = JX_NOTFOUND;
            return jx;
        }
        jx_right(jx);
    }
    return jx_down(jx);
}

char *jx_string_of(struct jx jx[], char const *key)
{
    if (error) return empty_string(jx);

    int pos = cursor(jx)->pos;
    jx_object_at(jx, key);
    delimit(jx);
    char *str = cstring(jx);
    rollback(jx, pos);
    return str;
}

long jx_long_of(struct jx jx[], char const *key)
{
    if (error) return 0;

    int pos = cursor(jx)->pos;
    jx_object_at(jx, key);
    long val = jx_as_long(jx);
    rollback(jx, pos);
    return val;
}

int jx_int_of(struct jx jx[], char const *key)
{
    if (error) return 0;

    int pos = cursor(jx)->pos;
    jx_object_at(jx, key);
    int val = jx_as_int(jx);
    rollback(jx, pos);
    return val;
}

unsigned long jx_ulong_of(struct jx jx[], char const *key)
{
    if (error) return 0;

    int pos = cursor(jx)->pos;
    jx_object_at(jx, key);
    unsigned long val = jx_as_ulong(jx);
    rollback(jx, pos);
    return val;
}

unsigned int jx_uint_of(struct jx jx[], char const *key)
{
    if (error) return 0;

    int pos = cursor(jx)->pos;
    jx_object_at(jx, key);
    unsigned int val = jx_as_uint(jx);
    rollback(jx, pos);
    return val;
}

char *jx_as_string(struct jx jx[])
{
    if (error) return empty_string(jx);

    delimit(jx);
    return cstring(jx);
}

long jx_as_long(struct jx jx[])
{
    if (error) return 0;

    delimit(jx);
    long val = zc_strto_long(cstring(jx), NULL, 10);
    input_errno();
    return val;
}

int jx_as_int(struct jx jx[])
{
    if (error) return 0;

    delimit(jx);
    int val = zc_strto_int(cstring(jx), NULL, 10);
    input_errno();
    return val;
}

unsigned long jx_as_ulong(struct jx jx[])
{
    if (error) return 0;

    delimit(jx);
    unsigned long val = zc_strto_ulong(cstring(jx), NULL, 10);
    input_errno();
    return val;
}

unsigned int jx_as_uint(struct jx jx[])
{
    if (error) return 0;

    delimit(jx);
    unsigned int val = zc_strto_uint(cstring(jx), NULL, 10);
    input_errno();
    return val;
}

static void sentinel_init(struct jx jx[])
{
    sentinel(jx)->type = JX_SENTINEL;
    sentinel(jx)->start = 0;
    sentinel(jx)->end = 1;
    sentinel(jx)->size = 0;
    sentinel(jx)->parent = get_parser(jx)->count;
    sentinel(jx)->prev = get_parser(jx)->count;
}
