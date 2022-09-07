#include "jx_parser.h"
#include "jx_error.h"
#include "jx_node.h"
#include "jx_type.h"
#include <assert.h>
#include <stdbool.h>

static int parse_primitive(struct jx_parser *parser, size_t length,
                           char const *json, size_t num_tokens,
                           struct jx_node *tokens);
static int parse_string(struct jx_parser *parser, const char *js,
                        const size_t len, struct jx_node *tokens,
                        const size_t num_tokens);
static int primitive_type(char c);
static void fill_node(struct jx_node *token, const int type, const int start,
                      const int end);

void parser_init(struct jx_parser *parser, int bits)
{
    parser->bits = bits;
    parser->nnodes = 0;
    parser->pos = 0;
    parser->toknext = 0;
    parser->toksuper = -1;
}

int open_bracket(char c, struct jx_parser *parser, int nnodes,
                 struct jx_node *nodes)
{
    if (nodes == NULL)
    {
        return JX_OK;
    }
    struct jx_node *node = node_alloc(parser, nnodes, nodes);
    if (node == NULL)
    {
        return JX_NOMEM;
    }
    if (parser->toksuper != -1)
    {
        struct jx_node *t = &nodes[parser->toksuper];
        /* In strict mode an object or array can't become a key */
        if (t->type == JX_OBJECT)
        {
            return JX_INVAL;
        }
        t->size++;
        node->parent = parser->toksuper;
    }
    node->type = (c == '{' ? JX_OBJECT : JX_ARRAY);
    node->start = parser->pos;
    parser->toksuper = parser->toknext - 1;
    return JX_OK;
}

int close_bracket(char c, struct jx_parser *parser, int nnodes,
                  struct jx_node *nodes)
{
    if (nodes == NULL)
    {
        return JX_OK;
    }
    int type = (c == '}' ? JX_OBJECT : JX_ARRAY);
    if (parser->toknext < 1)
    {
        return JX_INVAL;
    }
    struct jx_node *node = &nodes[parser->toknext - 1];
    for (;;)
    {
        if (node->start != -1 && node->end == -1)
        {
            if (node->type != type)
            {
                return JX_INVAL;
            }
            node->end = parser->pos + 1;
            parser->toksuper = node->parent;
            break;
        }
        if (node->parent == -1)
        {
            if (node->type != type || parser->toksuper == -1)
            {
                return JX_INVAL;
            }
            break;
        }
        node = &nodes[node->parent];
    }
    return JX_OK;
}

int parser_parse(struct jx_parser *parser, const size_t len, char *js,
                 int nnodes, struct jx_node *nodes)
{
    int rc = JX_OK;
    struct jx_node *node = NULL;
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
            if ((rc = open_bracket(c, parser, nnodes, nodes))) return rc;
            break;
        case '}':
        case ']':
            if ((rc = close_bracket(c, parser, nnodes, nodes))) return rc;
            break;
        case '\"':
            if ((rc = parse_string(parser, js, len, nodes, nnodes))) return rc;
            count++;
            if (parser->toksuper != -1 && nodes != NULL)
            {
                nodes[parser->toksuper].size++;
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
            if (nodes != NULL && parser->toksuper != -1 &&
                nodes[parser->toksuper].type != JX_ARRAY &&
                nodes[parser->toksuper].type != JX_OBJECT)
            {
                parser->toksuper = nodes[parser->toksuper].parent;
            }
            break;
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
            if (nodes != NULL && parser->toksuper != -1)
            {
                const struct jx_node *t = &nodes[parser->toksuper];
                if (t->type == JX_OBJECT ||
                    (t->type == JX_STRING && t->size != 0))
                {
                    return JX_INVAL;
                }
            }
            if ((rc = parse_primitive(parser, len, js, nnodes, nodes)))
                return rc;
            count++;
            if (parser->toksuper != -1 && nodes != NULL)
            {
                nodes[parser->toksuper].size++;
            }
            break;

        /* Unexpected char in strict mode */
        default:
            return JX_INVAL;
        }
    }

    if (nodes != NULL)
    {
        for (int i = parser->toknext - 1; i >= 0; i--)
        {
            /* Unmatched opened object or array */
            if (nodes[i].start != -1 && nodes[i].end == -1)
            {
                return JX_INVAL;
            }
        }
    }

    return count;
}

static int parse_primitive(struct jx_parser *parser, size_t len, char const *js,
                           size_t nnodes, struct jx_node *nodes)
{

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
            return JX_INVAL;
        }
    }
    /* In strict mode primitive must be followed by a comma/object/array */
    parser->pos = start;
    return JX_INVAL;

found:
    if (nodes == NULL)
    {
        parser->pos--;
        return JX_OK;
    }
    struct jx_node *node = node_alloc(parser, nnodes, nodes);
    if (node == NULL)
    {
        parser->pos = start;
        return JX_NOMEM;
    }
    fill_node(node, primitive_type(js[start]), start, parser->pos);
    node->parent = parser->toksuper;
    parser->pos--;
    return JX_OK;
}

static int parse_string(struct jx_parser *parser, const char *js,
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
                return JX_OK;
            }
            token = node_alloc(parser, num_tokens, tokens);
            if (token == NULL)
            {
                parser->pos = start;
                return JX_NOMEM;
            }
            fill_node(token, JX_STRING, start + 1, parser->pos);
            token->parent = parser->toksuper;
            return JX_OK;
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
                        return JX_INVAL;
                    }
                    parser->pos++;
                }
                parser->pos--;
                break;
            /* Unexpected symbol */
            default:
                parser->pos = start;
                return JX_INVAL;
            }
        }
    }
    parser->pos = start;
    return JX_INVAL;
}

static int primitive_type(char c)
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
        return JX_NUMBER;
    case 't':
    case 'f':
        return JX_BOOL;
        break;
    case 'n':
        return JX_NULL;
        break;
    default:
        assert(false);
    }
    assert(false);
}

static void fill_node(struct jx_node *token, const int type, const int start,
                      const int end)
{
    token->type = type;
    token->start = start;
    token->end = end;
    token->size = 0;
}
