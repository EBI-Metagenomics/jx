#include "jx.h"
#include "private.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define JSMN_STATIC
#include "libs/jsmn.h"

static inline struct jx_item const *as_item(struct jsmntok const *tok)
{
    union jx_union const *u = (union jx_union const *)tok;
    return &u->item;
}

static inline struct jsmntok const *as_tok(struct jx_item const *item)
{
    union jx_union const *u = (union jx_union const *)item;
    return &u->tok;
}

static void null_terminate(unsigned size, struct jsmntok toks[], char *data);
static void expand_types(unsigned size, struct jsmntok toks[],
                         char const *data);
static int string_hash(struct jx *jx, struct jx_item const *item);

struct jx *jx_new(unsigned bits)
{
    unsigned size = 1 << bits;
    struct jx *jx = malloc(sizeof(struct jx) + sizeof(struct jsmntok) * size);
    if (jx)
    {
        jx->first_errno = 0;
        jx->hash_table = malloc(sizeof(struct cco_hlist) * size);
        if (!jx->hash_table)
        {
            free(jx);
            return NULL;
        }
        jx->bits = bits;
        jx->size_max = size;
    }
    // __cco_hash_init(jx->hash_table, size);
    return jx;
}

void jx_del(struct jx *jx)
{
    free(jx->hash_table);
    free(jx);
}

static int convert_to_errno(int rc)
{
    if (rc == JSMN_ERROR_NOMEM) return ENOMEM;
    if (rc == JSMN_ERROR_INVAL) return EINVAL;
    if (rc == JSMN_ERROR_PART) return EINVAL;
    assert(false);
}

int jx_parse(struct jx *jx, char *str)
{
    jx->str = str;
    jx->first_errno = 0;
    jsmn_init(&jx->parser);
    size_t len = strlen(str);
    int rc = jsmn_parse(&jx->parser, str, len, jx->toks, jx->size_max);
    if (rc >= 0)
    {
        jx->size = (unsigned)rc;
        null_terminate(jx->size, jx->toks, str);
        expand_types(jx->size, jx->toks, jx->str);
    }
    else
    {
        jx->first_errno = convert_to_errno(rc);
    }
    return rc;
}

unsigned jx_nitems(struct jx const *jx) { return jx->size; }

struct jx_item *jx_root(struct jx const *jx)
{
    return (struct jx_item *)as_item(jx->toks);
}

int jx_errno(struct jx const *jx) { return jx->first_errno; }

char *jx_string(struct jx const *jx, struct jx_item const *item)
{
    if (jx->first_errno) return jx->str;
    return jx->str + as_tok(item)->start;
}

int64_t jx_int64(struct jx *jx, struct jx_item const *item)
{
    if (jx->first_errno) return 0;
    int64_t v = zc_strto_int64(jx_string(jx, item), NULL, 10);
    if (errno) jx->first_errno = errno;
    return v;
}

struct jx_object jx_object(struct jx *jx, struct jx_item *item)
{
    struct jx_object obj = {0};

    jx_assert_object(jx, item);
    if (jx->first_errno) return obj;

    unsigned nchild = jx_nchild(jx, item);
    struct jx_item *i = jx_next(jx, item);
    for (unsigned j = 0; j < nchild; ++j)
    {
        jx_assert_string(jx, i);
        jx_assert_nchild(jx, i, 1);
        if (jx->first_errno) return obj;
        obj.key = string_hash(jx, i);
        // __cco_hlist_add(&i->node,
        //                 &jx->hash_table[cco_hash_min(obj.key, jx->bits)]);
        item = jx_next(jx, i);
    }
    return obj;
}

static struct jx_item end = {JX_UNDEFINED, 0, 0, 0};

struct jx_item *jx_next(struct jx const *jx, struct jx_item *item)
{
    if (jx->first_errno) return &end;
    struct jsmntok const *next = as_tok(item);
    ++next;
    return (unsigned)(next - jx->toks) < jx_nitems(jx)
               ? (struct jx_item *)as_item(next)
               : &end;
}

bool jx_end(struct jx const *jx, struct jx_item const *item)
{
    if (jx->first_errno) return true;
    return item == &end;
}

unsigned jx_nchild(struct jx const *jx, struct jx_item const *item)
{
    if (jx->first_errno) return 0;
    return (unsigned)as_tok(item)->size;
}

bool jx_is_array(struct jx const *jx, struct jx_item const *item)
{
    if (jx->first_errno) return false;
    return as_tok(item)->type == JX_ARRAY;
}

bool jx_is_bool(struct jx const *jx, struct jx_item const *item)
{
    if (jx->first_errno) return false;
    return as_tok(item)->type == JX_BOOL;
}

bool jx_is_null(struct jx const *jx, struct jx_item const *item)
{
    if (jx->first_errno) return false;
    return as_tok(item)->type == JX_NULL;
}

bool jx_is_number(struct jx const *jx, struct jx_item const *item)
{
    if (jx->first_errno) return false;
    return as_tok(item)->type == JX_NUMBER;
}

bool jx_is_object(struct jx const *jx, struct jx_item const *item)
{
    if (jx->first_errno) return false;
    return as_tok(item)->type == JX_OBJECT;
}

bool jx_is_string(struct jx const *jx, struct jx_item const *item)
{
    if (jx->first_errno) return false;
    return as_tok(item)->type == JX_STRING;
}

void jx_assert_nitems(struct jx *jx, unsigned nitems)
{
    if (jx->first_errno) return;
    jx->first_errno = jx_nitems(jx) == nitems ? 0 : EINVAL;
}

void jx_assert_nchild(struct jx *jx, struct jx_item const *item,
                      unsigned nchild)
{
    if (jx->first_errno) return;
    jx->first_errno = jx_nchild(jx, item) == nchild ? 0 : EINVAL;
}

void jx_assert_array(struct jx *jx, struct jx_item const *item)
{
    if (jx->first_errno) return;
    jx->first_errno = jx_is_array(jx, item) ? 0 : EINVAL;
}

void jx_assert_bool(struct jx *jx, struct jx_item const *item)
{
    if (jx->first_errno) return;
    jx->first_errno = jx_is_bool(jx, item) ? 0 : EINVAL;
}

void jx_assert_null(struct jx *jx, struct jx_item const *item)
{
    if (jx->first_errno) return;
    jx->first_errno = jx_is_null(jx, item) ? 0 : EINVAL;
}

void jx_assert_number(struct jx *jx, struct jx_item const *item)
{
    if (jx->first_errno) return;
    jx->first_errno = jx_is_number(jx, item) ? 0 : EINVAL;
}

void jx_assert_object(struct jx *jx, struct jx_item const *item)
{
    if (jx->first_errno) return;
    jx->first_errno = jx_is_object(jx, item) ? 0 : EINVAL;
}

void jx_assert_string(struct jx *jx, struct jx_item const *item)
{
    if (jx->first_errno) return;
    jx->first_errno = jx_is_string(jx, item) ? 0 : EINVAL;
}

static void null_terminate(unsigned size, struct jsmntok toks[], char *data)
{
    for (unsigned i = 0; i < size; ++i)
    {
        if (toks[i].type == JSMN_STRING)
            data[toks[i].end] = '\0';
        else if (toks[i].type == JSMN_PRIMITIVE)
            data[toks[i].end] = '\0';
    }
}

static bool is_number(struct jsmntok const *tok, char const *data)
{
    char c = data[tok->start];
    return tok->type == JSMN_PRIMITIVE && c != 'n' && c != 't' && c != 'f';
}

static bool is_bool(struct jsmntok const *tok, char const *data)
{
    char c = data[tok->start];
    return tok->type == JSMN_PRIMITIVE && (c == 't' || c == 'f');
}

static bool is_null(struct jsmntok const *tok, char const *data)
{
    char c = data[tok->start];
    return tok->type == JSMN_PRIMITIVE && (c == 'n');
}

static void expand_types(unsigned size, struct jsmntok toks[], char const *data)
{
    for (unsigned i = 0; i < size; ++i)
    {
        if (toks[i].type == JSMN_UNDEFINED)
            toks[i].type = JX_UNDEFINED;
        else if (toks[i].type == JSMN_OBJECT)
            toks[i].type = JX_OBJECT;
        else if (toks[i].type == JSMN_ARRAY)
            toks[i].type = JX_ARRAY;
        else if (toks[i].type == JSMN_STRING)
            toks[i].type = JX_STRING;
        else if (toks[i].type == JSMN_PRIMITIVE)
        {
            if (is_null(toks + i, data))
                toks[i].type = JX_NULL;
            else if (is_bool(toks + i, data))
                toks[i].type = JX_BOOL;
            else if (is_number(toks + i, data))
                toks[i].type = JX_NUMBER;
            else
                assert(false);
        }
        else
            assert(false);
    }
}

static int string_hash(struct jx *jx, struct jx_item const *item)
{
    unsigned len = (unsigned)(item->end - item->start);
    union
    {
        XXH64_hash_t xxh64;
        int cco;
    } key = {.xxh64 = XXH3_64bits(jx_string(jx, item), len)};
    return key.cco;
}
