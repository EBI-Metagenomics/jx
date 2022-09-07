#include "jx_parser.h"
#include "jx_error.h"
#include "jx_node.h"
#include "jx_type.h"
#include <assert.h>
#include <stdbool.h>

static int parse_primitive(struct jx_parser *parser, size_t length,
                           char const *json, size_t num_tokens,
                           struct jx_node *tokens);
static int parse_string(struct jx_parser *parser, size_t len, const char *js,
                        size_t nnodes, struct jx_node *nodes);
static int primitive_type(char c);
static void fill_node(struct jx_node *token, const int type, const int start,
                      const int end);

void __jx_parser_init(struct jx_parser *parser, int alloc_size)
{
    parser->alloc_size = alloc_size;
    parser->size = 0;
    parser->pos = 0;
    parser->toknext = 0;
    parser->toksuper = -1;
}

int open_bracket(char c, struct jx_parser *parser, int nnodes,
                 struct jx_node *nodes)
{
    struct jx_node *node = __jx_node_alloc(parser, nnodes, nodes);
    if (node == NULL) return JX_NOMEM;
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

int check_umatched(struct jx_parser *parser, struct jx_node *nodes)
{
    for (int i = parser->toknext - 1; i >= 0; i--)
    {
        /* Unmatched opened object or array */
        if (nodes[i].start != -1 && nodes[i].end == -1) return JX_INVAL;
    }
    return JX_OK;
}

int close_bracket(char c, struct jx_parser *parser, int nnodes,
                  struct jx_node *nodes)
{
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

int __jx_parser_parse(struct jx_parser *parser, const size_t len, char *js,
                      int nnodes, struct jx_node *nodes)
{
    int rc = JX_OK;
    struct jx_node *node = NULL;
    parser->size = parser->toknext;

    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++)
    {
        char c;
        int type;

        c = js[parser->pos];
        switch (c)
        {
        case '{':
        case '[':
            parser->size++;
            if ((rc = open_bracket(c, parser, nnodes, nodes))) return rc;
            break;
        case '}':
        case ']':
            if ((rc = close_bracket(c, parser, nnodes, nodes))) return rc;
            break;
        case '\"':
            if ((rc = parse_string(parser, len, js, nnodes, nodes))) return rc;
            parser->size++;
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
            if (parser->toksuper != -1 &&
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
            if (parser->toksuper != -1)
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
            parser->size++;
            if (parser->toksuper != -1)
            {
                nodes[parser->toksuper].size++;
            }
            break;

        /* Unexpected char in strict mode */
        default:
            return JX_INVAL;
        }
    }

    if ((rc = check_umatched(parser, nodes))) return rc;

    return JX_OK;
}

static int parse_primitive(struct jx_parser *parser, size_t len, char const *js,
                           size_t nnodes, struct jx_node *nodes)
{
    int start = parser->pos;

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

found:;
    struct jx_node *node = __jx_node_alloc(parser, nnodes, nodes);
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

static int parse_string(struct jx_parser *parser, size_t len, const char *js,
                        size_t nnodes, struct jx_node *nodes)
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
            if (nodes == NULL)
            {
                return JX_OK;
            }
            token = __jx_node_alloc(parser, nnodes, nodes);
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
