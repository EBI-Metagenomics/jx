#include "jx.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#ifdef NULL
#undef NULL
#define NULL ((void *)0)
#endif

static struct jx_it *jsmn_alloc_token(struct jx *jx);
static int jsmn_parse_string(struct jx *jx);
static void jsmn_fill_token(struct jx_it *token, const int type,
                            const int start, const int end);
static int jsmn_parse_primitive(struct jx *jx);
static bool is_number(char c);
static inline bool is_bool(char c) { return c == 't' || c == 'f'; }
static inline bool is_null(char c) { return c == 'n'; }
static int primitive_type(char c);
static inline int relative_index(struct jx const *jx, struct jx_it *from,
                                 int to)
{
    return (int)(&jx->it[to] - from);
}
static inline int it_index(struct jx const *jx, struct jx_it const *it)
{
    return (int)(it - jx->it);
}

static inline void set_sentinel(struct jx_it *it, int previous)
{
    it->start = -1;
    it->end = -1;
    it->size = 0;
    it->type = JX_UNDEF;
    it->parent = 0;
    it->previous = previous;
}

struct jx_it *jx_parse(struct jx *jx, char const *json)
{
    jx->json = json;
    jx->ferrno = 0;
    jx->parser.pos = 0;
    jx->parser.toknext = 0;
    jx->parser.toksuper = -1;

    jx->length = strlen(json);

    int r;
    int i;
    struct jx_it *toks = jx->it;
    struct jx_it *token;
    int count = 0;

    for (; jx->parser.pos < jx->length && json[jx->parser.pos] != '\0';
         jx->parser.pos++)
    {
        char c;
        int type;

        c = json[jx->parser.pos];
        switch (c)
        {
        case '{':
        case '[':
            count++;
            if (toks == NULL)
            {
                break;
            }
            token = jsmn_alloc_token(jx);
            if (token == NULL)
            {
                jx->ferrno = ENOMEM;
                set_sentinel(&jx->it[0], 0);
                return &jx->it[0];
            }
            if (jx->parser.toksuper != -1)
            {
                struct jx_it *t = &toks[jx->parser.toksuper];
                /* In strict mode an object or array can't become a key */
                if (t->type == JX_OBJECT)
                {
                    jx->ferrno = EINVAL;
                    set_sentinel(&jx->it[0], 0);
                    return &jx->it[0];
                }
                t->size++;
                token->parent = jx->parser.toksuper;
            }
            token->type = (c == '{' ? JX_OBJECT : JX_ARRAY);
            token->start = jx->parser.pos;
            jx->parser.toksuper = jx->parser.toknext - 1;
            break;
        case '}':
        case ']':
            if (toks == NULL)
            {
                break;
            }
            type = (c == '}' ? JX_OBJECT : JX_ARRAY);
            if (jx->parser.toknext < 1)
            {
                jx->ferrno = EINVAL;
                set_sentinel(&jx->it[0], 0);
                return &jx->it[0];
            }
            token = &toks[jx->parser.toknext - 1];
            for (;;)
            {
                if (token->start != -1 && token->end == -1)
                {
                    if (token->type != type)
                    {
                        jx->ferrno = EINVAL;
                        set_sentinel(&jx->it[0], 0);
                        return &jx->it[0];
                    }
                    token->end = jx->parser.pos + 1;
                    jx->parser.toksuper = token->parent;
                    break;
                }
                if (token->parent == -1)
                {
                    if (token->type != type || jx->parser.toksuper == -1)
                    {
                        jx->ferrno = EINVAL;
                        set_sentinel(&jx->it[0], 0);
                        return &jx->it[0];
                    }
                    break;
                }
                token = &toks[token->parent];
            }
            break;
        case '\"':
            jx->ferrno = jsmn_parse_string(jx);
            if (jx->ferrno < 0)
            {
                set_sentinel(&jx->it[0], 0);
                return &jx->it[0];
            }
            count++;
            if (jx->parser.toksuper != -1 && toks != NULL)
            {
                toks[jx->parser.toksuper].size++;
            }
            break;
        case '\t':
        case '\r':
        case '\n':
        case ' ':
            break;
        case ':':
            jx->parser.toksuper = jx->parser.toknext - 1;
            break;
        case ',':
            if (toks != NULL && jx->parser.toksuper != -1 &&
                toks[jx->parser.toksuper].type != JX_ARRAY &&
                toks[jx->parser.toksuper].type != JX_OBJECT)
            {
                jx->parser.toksuper = toks[jx->parser.toksuper].parent;
            }
            break;
        /* In strict mode primitives are: numbers, booleans, and null */
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 't':
        case 'f':
        case 'n':
            /* And they must not be keys of the object */
            if (toks != NULL && jx->parser.toksuper != -1)
            {
                const struct jx_it *t = &toks[jx->parser.toksuper];
                if (t->type == JX_OBJECT ||
                    (t->type == JX_STRING && t->size != 0))
                {
                    jx->ferrno = EINVAL;
                    set_sentinel(&jx->it[0], 0);
                    return &jx->it[0];
                }
            }
            r = jsmn_parse_primitive(jx);
            if (r < 0)
            {
                jx->ferrno = r;
                set_sentinel(&jx->it[0], 0);
                return &jx->it[0];
            }
            count++;
            if (jx->parser.toksuper != -1 && toks != NULL)
            {
                toks[jx->parser.toksuper].size++;
            }
            break;

        /* Unexpected char in strict mode */
        default:
            jx->ferrno = EINVAL;
            set_sentinel(&jx->it[0], 0);
            return &jx->it[0];
        }
    }

    jx->nitems = (unsigned)count;
    if (toks != NULL)
    {
        for (i = jx->parser.toknext - 1; i >= 0; i--)
        {
            /* Unmatched opened object or array */
            if (toks[i].start != -1 && toks[i].end == -1)
            {
                jx->ferrno = EINVAL;
                set_sentinel(&jx->it[0], 0);
                return &jx->it[0];
            }
        }

        for (i = 0; i < jx->nitems; ++i)
            jx->it[i].parent = relative_index(jx, &jx->it[i], jx->it[i].parent);
    }

    jx->it[jx->nitems].start = -1;
    jx->it[jx->nitems].end = -1;
    jx->it[jx->nitems].size = 0;
    jx->it[jx->nitems].type = JX_UNDEF;
    jx->it[jx->nitems].parent = 0;
    return jx->it;
}

int jx_errno(struct jx const *jx) { return jx->ferrno; }

int jx_type(struct jx_it const *it) { return it->type; }

struct jx_it *jx_back(struct jx *jx, struct jx_it *it)
{
    return &jx->it[it_index(jx, it) + it->previous];
}

struct jx_it *jx_down(struct jx *jx, struct jx_it *it)
{
    if (it->type == JX_UNDEF) return it;
    if (it->size == 0)
    {
        struct jx_it *sentinel = &jx->it[jx->nitems];
        set_sentinel(sentinel, relative_index(jx, sentinel, it_index(jx, it)));
        return sentinel;
    }
    struct jx_it *next = it + 1;
    next->previous = relative_index(jx, next, it_index(jx, it));
    return next;
}

struct jx_it *jx_next(struct jx *jx, struct jx_it *it)
{
    if (it->type == JX_UNDEF) return it;
    it->previous = relative_index(jx, it + 1, it_index(jx, it));
    return it + 1;
}

struct jx_it *jx_right(struct jx *jx, struct jx_it *it)
{
    struct jx_it *next = NULL;

    goto enter;
    while (jx_up(jx, next) != jx_up(jx, it))
    {
        if (next->type == JX_UNDEF)
        {
            set_sentinel(next, 0);
            break;
        }
    enter:
        next = jx_next(jx, it);
    }
    return next;
}

struct jx_it *jx_up(struct jx *jx, struct jx_it *it) { return it + it->parent; }

struct jx_array *jx_as_array(struct jx *jx, struct jx_it *it)
{
    if (!jx->ferrno && jx_type(it) != JX_ARRAY) jx->ferrno = EINVAL;
    return (struct jx_array *)it;
}

struct jx_object *jx_as_object(struct jx *jx, struct jx_it *it)
{
    if (!jx->ferrno && jx_type(it) != JX_OBJECT) jx->ferrno = EINVAL;
    return (struct jx_object *)it;
}

struct jx_string *jx_as_string(struct jx *jx, struct jx_it *it)
{
    if (!jx->ferrno && jx_type(it) != JX_STRING) jx->ferrno = EINVAL;
    return (struct jx_string *)it;
}

char *jx_strdup(struct jx *jx, struct jx_string const *str)
{
    if (jx->ferrno) return NULL;
    char *dst = malloc(jx_strlen(jx, str) + 1);
    if (!dst)
    {
        jx->ferrno = ENOMEM;
        return NULL;
    }
    memcpy(dst, jx->json + str->start, jx_strlen(jx, str));
    dst[jx_strlen(jx, str)] = '\0';
    return dst;
}

void jx_strcpy(struct jx *jx, char *dst, struct jx_string const *str,
               size_t dsize)
{
    unsigned len = jx_strlen(jx, str);
    if (len + 1 > dsize)
    {
        if (dsize > 0) dst[0] = '\0';
        jx->ferrno = ENOMEM;
        return;
    }
    memcpy(dst, jx->json + str->start, len);
    dst[len] = '\0';
}

size_t jx_strlen(struct jx *jx, struct jx_string const *str)
{
    return (str->end - str->start);
}

/**
 * Allocates a fresh unused token from the token pool.
 */
static struct jx_it *jsmn_alloc_token(struct jx *jx)
{
    struct jx_it *tok;
    /* the last item is a sentinel (end type) */
    if (jx->parser.toknext > jx->max_nitems)
    {
        return NULL;
    }
    tok = &jx->it[jx->parser.toknext++];
    tok->start = tok->end = -1;
    tok->size = 0;
    tok->parent = -1;
    tok->previous = 0;
    return tok;
}

/**
 * Fills next token with JSON string.
 */
static int jsmn_parse_string(struct jx *jx)
{
    struct jx_it *token;

    int start = jx->parser.pos;

    /* Skip starting quote */
    jx->parser.pos++;

    for (; jx->parser.pos < jx->length && jx->json[jx->parser.pos] != '\0';
         jx->parser.pos++)
    {
        char c = jx->json[jx->parser.pos];

        /* Quote: end of string */
        if (c == '\"')
        {
            if (jx->it == NULL)
            {
                return 0;
            }
            token = jsmn_alloc_token(jx);
            if (token == NULL)
            {
                jx->parser.pos = start;
                return ENOMEM;
            }
            jsmn_fill_token(token, JX_STRING, start + 1, jx->parser.pos);
            token->parent = jx->parser.toksuper;
            return 0;
        }

        /* Backslash: Quoted symbol expected */
        if (c == '\\' && jx->parser.pos + 1 < jx->length)
        {
            int i;
            jx->parser.pos++;
            switch (jx->json[jx->parser.pos])
            {
            /* Allowed escaped symbols */
            case '\"':
            case '/':
            case '\\':
            case 'b':
            case 'f':
            case 'r':
            case 'n':
            case 't':
                break;
            /* Allows escaped symbol \uXXXX */
            case 'u':
                jx->parser.pos++;
                for (i = 0; i < 4 && jx->parser.pos < jx->length &&
                            jx->json[jx->parser.pos] != '\0';
                     i++)
                {
                    /* If it isn't a hex character we have an error */
                    if (!((jx->json[jx->parser.pos] >= 48 &&
                           jx->json[jx->parser.pos] <= 57) || /* 0-9 */
                          (jx->json[jx->parser.pos] >= 65 &&
                           jx->json[jx->parser.pos] <= 70) || /* A-F */
                          (jx->json[jx->parser.pos] >= 97 &&
                           jx->json[jx->parser.pos] <= 102)))
                    { /* a-f */
                        jx->parser.pos = start;
                        return EINVAL;
                    }
                    jx->parser.pos++;
                }
                jx->parser.pos--;
                break;
            /* Unexpected symbol */
            default:
                jx->parser.pos = start;
                return EINVAL;
            }
        }
    }
    jx->parser.pos = start;
    return EINVAL;
}

static void jsmn_fill_token(struct jx_it *token, const int type,
                            const int start, const int end)
{
    token->type = type;
    token->start = start;
    token->end = end;
    token->size = 0;
}

/**
 * Fills next available token with JSON primitive.
 */
static int jsmn_parse_primitive(struct jx *jx)
{
    struct jx_it *token;
    int start = jx->parser.pos;

    for (; jx->parser.pos < jx->length && jx->json[jx->parser.pos] != '\0';
         jx->parser.pos++)
    {
        switch (jx->json[jx->parser.pos])
        {
        case '\t':
        case '\r':
        case '\n':
        case ' ':
        case ',':
        case ']':
        case '}':
            goto found;
        default:
            /* to quiet a warning from gcc*/
            break;
        }
        if (jx->json[jx->parser.pos] < 32 || jx->json[jx->parser.pos] >= 127)
        {
            jx->parser.pos = start;
            return EINVAL;
        }
    }
    /* In strict mode primitive must be followed by a comma/object/array */
    jx->parser.pos = start;
    return EINVAL;

found:
    if (jx->it == NULL)
    {
        jx->parser.pos--;
        return 0;
    }
    token = jsmn_alloc_token(jx);
    if (token == NULL)
    {
        jx->parser.pos = start;
        return ENOMEM;
    }
    jsmn_fill_token(token, primitive_type(jx->json[start]), start,
                    jx->parser.pos);
    token->parent = jx->parser.toksuper;
    jx->parser.pos--;
    return 0;
}

static bool is_number(char c)
{
    switch (c)
    {
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return true;
    default:
        break;
    };
    return false;
}

static int primitive_type(char c)
{
    if (is_null(c)) return JX_NULL;
    if (is_bool(c)) return JX_BOOL;
    if (is_number(c)) return JX_NUMBER;
    assert(false);
    return 0;
}
