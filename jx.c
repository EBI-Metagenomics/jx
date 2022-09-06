#include "jx.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

enum
{
    JSMN_PRIMITIVE = 1 << 3
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

static inline struct jx_parser *get_parser(struct jx jx[])
{
    return &jx[0].parser;
}

static inline struct jx_cursor *get_cursor(struct jx jx[])
{
    return &jx[1].cursor;
}

static inline struct jx_node *get_node(struct jx jx[], int idx)
{
    return &jx[2 + idx].node;
}

void jx_init(struct jx jx[], int bits) { get_parser(jx)->bits = bits; }

static void convert_types(int size, struct jx jx[])
{
    struct jx_parser *parser = get_parser(jx);
    struct jx_cursor *cursor = get_cursor(jx);
    for (int i = 0; i < size; ++i)
    {
        if (get_node(jx, i)->type == JSMN_PRIMITIVE)
        {
            switch (cursor->json[get_node(jx, i)->start])
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
                get_node(jx, i)->type = JX_NUMBER;
                break;
            case 't':
            case 'f':
                get_node(jx, i)->type = JX_BOOL;
                break;
            case 'n':
                get_node(jx, i)->type = JX_NULL;
                break;
            default:
                assert(false);
            }
        }
    }
}

int jx_parse(struct jx jx[], char *json)
{
    struct jx_parser *parser = get_parser(jx);
    get_cursor(jx)->json = json;
    struct jx_node *node = get_node(jx, 0);
    int rc = jsmn_parse(parser, json, strlen(json), node, 1 << parser->bits);
    if (rc < 0) return rc;
    convert_types(rc, jx);
    return rc;
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
