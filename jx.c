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

static thread_local int errno_local = 0;

enum offset
{
    PARSER_OFFSET = 0,
    CURSOR_OFFSET = 1,
    NODE_OFFSET = 2,
};

#define PARSER(jx) ((jx)[PARSER_OFFSET].parser)
#define cursor(jx) ((jx)[CURSOR_OFFSET].cursor)
#define nodes(jx) (&((jx)[NODE_OFFSET].node))
#define cnode(jx) nodes(jx)[cursor(jx).pos]
#define sentinel(jx) nodes(jx)[PARSER(jx).nnodes]

void jx_init(struct jx jx[], int bits)
{
    parser_init(&PARSER(jx), bits);
    cursor(jx).pos = 0;
}

static void sentinel_init(struct jx jx[])
{
    sentinel(jx).type = JX_SENTINEL;
    sentinel(jx).start = 0;
    sentinel(jx).end = 1;
    sentinel(jx).size = 0;
    sentinel(jx).parent = PARSER(jx).nnodes;
    sentinel(jx).prev = PARSER(jx).nnodes;
}

int jx_parse(struct jx jx[], char *json)
{
    cursor(jx).length = strlen(json);
    cursor(jx).json = json;
    struct jx_parser *parser = &PARSER(jx);
    unsigned n = 1 << parser->bits;
    int rc = parser_parse(parser, strlen(json), json, n, nodes(jx));
    if (rc < 0) return rc;
    parser->nnodes = rc;
    sentinel_init(jx);
    if (parser->nnodes > 0) cnode(jx).parent = -1;
    return rc;
}

int jx_error(void) { return errno_local; }

void jx_clear(void) { errno_local = 0; }

int jx_type(struct jx const jx[])
{
    return nodes((struct jx *)jx)[cursor((struct jx *)jx).pos].type;
}

struct jx *jx_back(struct jx jx[])
{
    cursor(jx).pos = cnode(jx).prev;
    return jx;
}

static struct jx *setup_sentinel(struct jx jx[])
{
    nodes(jx)[PARSER(jx).nnodes].prev = cursor(jx).pos;
    cursor(jx).pos = PARSER(jx).nnodes;
    return jx;
}

struct jx *jx_down(struct jx jx[])
{
    if (jx_type(jx) == JX_SENTINEL) return jx;

    if (cnode(jx).size == 0) return setup_sentinel(jx);

    return jx_next(jx);
}

struct jx *jx_next(struct jx jx[])
{
    if (jx_type(jx) == JX_SENTINEL) return jx;

    if (cursor(jx).pos + 1 >= PARSER(jx).nnodes) return setup_sentinel(jx);

    nodes(jx)[cursor(jx).pos + 1].prev = cursor(jx).pos;
    cursor(jx).pos++;
    return jx;
}

static struct jx *rollback(struct jx jx[], int pos)
{
    while (cursor(jx_back(jx)).pos != pos)
        ;
    return jx;
}

struct jx *jx_right(struct jx jx[])
{
    if (jx_type(jx) == JX_SENTINEL) return jx;

    int parent = cnode(jx).parent;
    int pos = cursor(jx).pos;
    if (parent == -1) return setup_sentinel(jx);
    while (parent != cnode(jx_next(jx)).parent)
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

    int parent = cnode(jx).parent;
    if (parent == -1) return setup_sentinel(jx);

    nodes(jx)[parent].prev = cursor(jx).pos;
    cursor(jx).pos = parent;
    return jx;
}

// struct jx *jx_array_at(struct jx jx[], int idx) {}

struct jx *jx_object_at(struct jx jx[], char const *key)
{
    if (jx_type(jx) != JX_OBJECT)
    {
        errno_local = EINVAL;
        return jx;
    }

    int pos = cursor(jx).pos;
    jx_down(jx);
    while (strcmp(jx_as_string(jx), key))
    {
        if (jx_type(jx) == JX_SENTINEL)
        {
            rollback(jx, pos);
            errno_local = EINVAL;
            return jx;
        }
        jx_right(jx);
    }
    return jx_down(jx);
}

static inline char *empty_string(struct jx jx[])
{
    return &cursor(jx).json[cursor(jx).length];
}

char *jx_string_of(struct jx jx[], char const *key)
{
    if (errno_local) return empty_string(jx);

    jx_object_at(jx, key);
    cursor(jx).json[cnode(jx).end] = '\0';
    char *str = &cursor(jx).json[cnode(jx).start];
    jx_up(jx);
    jx_up(jx);
    return str;
}

int64_t jx_int64_of(struct jx jx[], char const *key)
{
    if (errno_local) return 0;

    int save_errno = errno;
    jx_object_at(jx, key);
    cursor(jx).json[cnode(jx).end] = '\0';
    int64_t val = zc_strto_int64(&cursor(jx).json[cnode(jx).start], NULL, 10);
    errno_local = errno;
    errno = save_errno;
    jx_up(jx);
    jx_up(jx);
    return val;
}

char *jx_as_string(struct jx jx[])
{
    if (errno_local) return empty_string(jx);

    cursor(jx).json[cnode(jx).end] = '\0';
    return &cursor(jx).json[cnode(jx).start];
}

int jx_as_int(struct jx jx[])
{
    if (errno_local) return 0;

    cursor(jx).json[cnode(jx).end] = '\0';
    int val = zc_strto_int(&cursor(jx).json[cnode(jx).start], NULL, 10);
    errno_local = errno;
    errno = 0;
    return val;
}
