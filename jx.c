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
    union jx_union *u = (union jx_union *)tok;
    return &u->item;
}

static void null_terminate(unsigned size, struct jsmntok toks[], char *data);
static void expand_types(unsigned size, struct jsmntok toks[],
                         char const *data);

struct jx *jx_new(unsigned nitems)
{
    struct jx *jx = malloc(sizeof(struct jx) + sizeof(struct jsmntok) * nitems);
    if (jx)
    {
        jx->first_errno = 0;
        jx->size_max = nitems;
    }
    return jx;
}

void jx_del(struct jx *jx) { free(jx); }

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

void jx_assert_nitems(struct jx *jx, unsigned nitems)
{
    if (jx->first_errno) return;
    jx->first_errno = jx_nitems(jx) == nitems ? 0 : EINVAL;
}

struct jx_item const *jx_root(struct jx const *jx) { return as_item(jx->toks); }

int jx_errno(struct jx const *jx) { return jx->first_errno; }

static inline struct jsmntok const *as_tok(struct jx_item const *item)
{
    union jx_union *u = (union jx_union *)item;
    return &u->tok;
}

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

static struct jx_item end = {JX_UNDEFINED, 0, 0, 0};

struct jx_item const *jx_next(struct jx const *jx, struct jx_item const *item)
{
    if (jx->first_errno) return &end;
    struct jsmntok const *next = as_tok(item + 1);
    return (unsigned)(next - jx->toks) < jx_nitems(jx) ? as_item(next) : &end;
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
