#include "jx.h"
#include "compiler.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

static thread_local int jx_errno_local = 0;

enum
{
    JSMN_PRIMITIVE = 1 << 3
};

enum
{
    PARSER_OFFSET = 0,
    CURSOR_OFFSET = 1,
    NODE_OFFSET = 2,
};

enum
{
    /* Not enough tokens were provided */
    JSMN_ERROR_NOMEM = -1,
    /* Invalid character inside JSON string */
    JSMN_ERROR_INVAL = -2,
    /* The string is not a full JSON packet, more bytes expected */
    JSMN_ERROR_PART = -3
};

/**
 * Create JSON parser over an array of tokens
 */
void jsmn_init(struct jx_parser *parser);

/**
 * Run JSON parser. It parses a JSON data string into and array of tokens, each
 * describing
 * a single JSON object.
 */
int jsmn_parse(struct jx_parser *parser, const char *js, const size_t len,
               struct jx_node *tokens, const unsigned int num_tokens);

#define PARSER(jx) ((jx)[PARSER_OFFSET].parser)
#define cursor(jx) ((jx)[CURSOR_OFFSET].cursor)
#define nodes(jx) (&((jx)[NODE_OFFSET].node))
#define sentinel(jx) nodes(jx)[PARSER(jx).nnodes]

void jx_init(struct jx jx[], int bits)
{
    PARSER(jx).bits = bits;
    cursor(jx).pos = 0;
}

static void convert_types(struct jx jx[])
{
    for (int i = 0; i < PARSER(jx).nnodes; ++i)
    {
        if (nodes(jx)[i].type == JSMN_PRIMITIVE)
        {
            switch (cursor(jx).json[nodes(jx)[i].start])
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
                nodes(jx)[i].type = JX_NUMBER;
                break;
            case 't':
            case 'f':
                nodes(jx)[i].type = JX_BOOL;
                break;
            case 'n':
                nodes(jx)[i].type = JX_NULL;
                break;
            default:
                assert(false);
            }
        }
    }
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
    cursor(jx).json = json;
    struct jx_parser *parser = &PARSER(jx);
    unsigned n = 1 << parser->bits;
    int rc = jsmn_parse(parser, json, strlen(json), nodes(jx), n);
    if (rc < 0) return rc;
    parser->nnodes = rc;
    sentinel_init(jx);
    convert_types(jx);
    return rc;
}

int jx_errno(void) { return jx_errno_local; }

void jx_clear(void) { jx_errno_local = 0; }

int jx_type(struct jx const jx[])
{
    return nodes((struct jx *)jx)[cursor((struct jx *)jx).pos].type;
}

struct jx *jx_back(struct jx jx[])
{
    cursor(jx).pos = nodes(jx)[cursor(jx).pos].prev;
    return jx;
}

struct jx *jx_next(struct jx jx[])
{
    if (jx_type(jx) == JX_SENTINEL) return jx;

    if (cursor(jx).pos + 1 >= PARSER(jx).nnodes)
    {
        nodes(jx)[PARSER(jx).nnodes].prev = cursor(jx).pos;
        cursor(jx).pos = PARSER(jx).nnodes;
    }
    else
    {
        nodes(jx)[cursor(jx).pos + 1].prev = cursor(jx).pos;
        cursor(jx).pos++;
    }
    return jx;
}

/**
 * Allocates a fresh unused token from the token pool.
 */
static struct jx_node *jsmn_alloc_token(struct jx_parser *parser,
                                        struct jx_node *tokens,
                                        const size_t num_tokens)
{
    struct jx_node *tok;
    if (parser->toknext >= num_tokens)
    {
        return NULL;
    }
    tok = &tokens[parser->toknext++];
    tok->start = tok->end = -1;
    tok->size = 0;
    tok->parent = -1;
    return tok;
}

/**
 * Fills token type and boundaries.
 */
static void jsmn_fill_token(struct jx_node *token, const int type,
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
static int jsmn_parse_primitive(struct jx_parser *parser, const char *js,
                                const size_t len, struct jx_node *tokens,
                                const size_t num_tokens)
{
    struct jx_node *token;
    int start;

    start = parser->pos;

    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++)
    {
        switch (js[parser->pos])
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
        if (js[parser->pos] < 32 || js[parser->pos] >= 127)
        {
            parser->pos = start;
            return JSMN_ERROR_INVAL;
        }
    }
    /* In strict mode primitive must be followed by a comma/object/array */
    parser->pos = start;
    return JSMN_ERROR_PART;

found:
    if (tokens == NULL)
    {
        parser->pos--;
        return 0;
    }
    token = jsmn_alloc_token(parser, tokens, num_tokens);
    if (token == NULL)
    {
        parser->pos = start;
        return JSMN_ERROR_NOMEM;
    }
    jsmn_fill_token(token, JSMN_PRIMITIVE, start, parser->pos);
    token->parent = parser->toksuper;
    parser->pos--;
    return 0;
}

/**
 * Fills next token with JSON string.
 */
static int jsmn_parse_string(struct jx_parser *parser, const char *js,
                             const size_t len, struct jx_node *tokens,
                             const size_t num_tokens)
{
    struct jx_node *token;

    int start = parser->pos;

    /* Skip starting quote */
    parser->pos++;

    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++)
    {
        char c = js[parser->pos];

        /* Quote: end of string */
        if (c == '\"')
        {
            if (tokens == NULL)
            {
                return 0;
            }
            token = jsmn_alloc_token(parser, tokens, num_tokens);
            if (token == NULL)
            {
                parser->pos = start;
                return JSMN_ERROR_NOMEM;
            }
            jsmn_fill_token(token, JX_STRING, start + 1, parser->pos);
            token->parent = parser->toksuper;
            return 0;
        }

        /* Backslash: Quoted symbol expected */
        if (c == '\\' && parser->pos + 1 < len)
        {
            int i;
            parser->pos++;
            switch (js[parser->pos])
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
                parser->pos++;
                for (i = 0;
                     i < 4 && parser->pos < len && js[parser->pos] != '\0'; i++)
                {
                    /* If it isn't a hex character we have an error */
                    if (!((js[parser->pos] >= 48 &&
                           js[parser->pos] <= 57) || /* 0-9 */
                          (js[parser->pos] >= 65 &&
                           js[parser->pos] <= 70) || /* A-F */
                          (js[parser->pos] >= 97 && js[parser->pos] <= 102)))
                    { /* a-f */
                        parser->pos = start;
                        return JSMN_ERROR_INVAL;
                    }
                    parser->pos++;
                }
                parser->pos--;
                break;
            /* Unexpected symbol */
            default:
                parser->pos = start;
                return JSMN_ERROR_INVAL;
            }
        }
    }
    parser->pos = start;
    return JSMN_ERROR_PART;
}

/**
 * Parse JSON string and fill tokens.
 */
int jsmn_parse(struct jx_parser *parser, const char *js, const size_t len,
               struct jx_node *tokens, const unsigned int num_tokens)
{
    int r;
    int i;
    struct jx_node *token;
    int count = parser->toknext;

    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++)
    {
        char c;
        int type;

        c = js[parser->pos];
        switch (c)
        {
        case '{':
        case '[':
            count++;
            if (tokens == NULL)
            {
                break;
            }
            token = jsmn_alloc_token(parser, tokens, num_tokens);
            if (token == NULL)
            {
                return JSMN_ERROR_NOMEM;
            }
            if (parser->toksuper != -1)
            {
                struct jx_node *t = &tokens[parser->toksuper];
                /* In strict mode an object or array can't become a key */
                if (t->type == JX_OBJECT)
                {
                    return JSMN_ERROR_INVAL;
                }
                t->size++;
                token->parent = parser->toksuper;
            }
            token->type = (c == '{' ? JX_OBJECT : JX_ARRAY);
            token->start = parser->pos;
            parser->toksuper = parser->toknext - 1;
            break;
        case '}':
        case ']':
            if (tokens == NULL)
            {
                break;
            }
            type = (c == '}' ? JX_OBJECT : JX_ARRAY);
            if (parser->toknext < 1)
            {
                return JSMN_ERROR_INVAL;
            }
            token = &tokens[parser->toknext - 1];
            for (;;)
            {
                if (token->start != -1 && token->end == -1)
                {
                    if (token->type != type)
                    {
                        return JSMN_ERROR_INVAL;
                    }
                    token->end = parser->pos + 1;
                    parser->toksuper = token->parent;
                    break;
                }
                if (token->parent == -1)
                {
                    if (token->type != type || parser->toksuper == -1)
                    {
                        return JSMN_ERROR_INVAL;
                    }
                    break;
                }
                token = &tokens[token->parent];
            }
            break;
        case '\"':
            r = jsmn_parse_string(parser, js, len, tokens, num_tokens);
            if (r < 0)
            {
                return r;
            }
            count++;
            if (parser->toksuper != -1 && tokens != NULL)
            {
                tokens[parser->toksuper].size++;
            }
            break;
        case '\t':
        case '\r':
        case '\n':
        case ' ':
            break;
        case ':':
            parser->toksuper = parser->toknext - 1;
            break;
        case ',':
            if (tokens != NULL && parser->toksuper != -1 &&
                tokens[parser->toksuper].type != JX_ARRAY &&
                tokens[parser->toksuper].type != JX_OBJECT)
            {
                parser->toksuper = tokens[parser->toksuper].parent;
            }
            break;
        /* In strict mode primitives are: numbers and booleans */
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
            if (tokens != NULL && parser->toksuper != -1)
            {
                const struct jx_node *t = &tokens[parser->toksuper];
                if (t->type == JX_OBJECT ||
                    (t->type == JX_STRING && t->size != 0))
                {
                    return JSMN_ERROR_INVAL;
                }
            }
            r = jsmn_parse_primitive(parser, js, len, tokens, num_tokens);
            if (r < 0)
            {
                return r;
            }
            count++;
            if (parser->toksuper != -1 && tokens != NULL)
            {
                tokens[parser->toksuper].size++;
            }
            break;

        /* Unexpected char in strict mode */
        default:
            return JSMN_ERROR_INVAL;
        }
    }

    if (tokens != NULL)
    {
        for (i = parser->toknext - 1; i >= 0; i--)
        {
            /* Unmatched opened object or array */
            if (tokens[i].start != -1 && tokens[i].end == -1)
            {
                return JSMN_ERROR_PART;
            }
        }
    }

    return count;
}

/**
 * Creates a new parser based over a given buffer with an array of tokens
 * available.
 */
void jsmn_init(struct jx_parser *parser)
{
    parser->pos = 0;
    parser->toknext = 0;
    parser->toksuper = -1;
}
