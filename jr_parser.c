#include "jr_parser.h"
#include "jr_error.h"
#include "jr_node.h"
#include "jr_type.h"
/* meld-cut-here */
#include <assert.h>
#include <stdbool.h>

static int parse_primitive(struct jr_parser *parser, int length,
                           char const *json, int num_tokens,
                           struct jr_node *tokens);
static int parse_string(struct jr_parser *parser, int len, const char *js,
                        int nnodes, struct jr_node *nodes);
static int primitive_type(char c);
static void fill_node(struct jr_node *token, const int type, const int start,
                      const int end);
static int open_bracket(char c, struct jr_parser *parser, int nnodes,
                        struct jr_node *nodes);
static int check_umatched(struct jr_parser *parser, struct jr_node *nodes);
static int close_bracket(char c, struct jr_parser *parser,
                         struct jr_node *nodes);

extern void jr_parser_init(struct jr_parser *parser, int alloc_size)
{
    parser->alloc_size = alloc_size;
    parser->size = 0;
    parser->pos = 0;
    parser->toknext = 0;
    parser->toksuper = -1;
}

extern void jr_parser_reset(struct jr_parser *parser)
{
    parser->size = 0;
    parser->pos = 0;
    parser->toknext = 0;
    parser->toksuper = -1;
}

extern int jr_parser_parse(struct jr_parser *parser, const int len, char *js,
                           int nnodes, struct jr_node *nodes)
{
    int rc = JR_OK;
    parser->size = parser->toknext;

    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++)
    {
        char c = js[parser->pos];
        switch (c)
        {
        case '{':
        case '[':
            parser->size++;
            if ((rc = open_bracket(c, parser, nnodes, nodes))) return rc;
            break;
        case '}':
        case ']':
            if ((rc = close_bracket(c, parser, nodes))) return rc;
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
                nodes[parser->toksuper].type != JR_ARRAY &&
                nodes[parser->toksuper].type != JR_OBJECT)
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
                const struct jr_node *t = &nodes[parser->toksuper];
                if (t->type == JR_OBJECT ||
                    (t->type == JR_STRING && t->size != 0))
                {
                    return JR_INVAL;
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
            return JR_INVAL;
        }
    }

    if ((rc = check_umatched(parser, nodes))) return rc;

    return JR_OK;
}

static int parse_primitive(struct jr_parser *parser, int len, char const *js,
                           int nnodes, struct jr_node *nodes)
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
            return JR_INVAL;
        }
    }
    /* In strict mode primitive must be followed by a comma/object/array */
    parser->pos = start;
    return JR_INVAL;

found:;
    struct jr_node *node = __jr_node_alloc(parser, nnodes, nodes);
    if (node == NULL)
    {
        parser->pos = start;
        return JR_NOMEM;
    }
    fill_node(node, primitive_type(js[start]), start, parser->pos);
    node->parent = parser->toksuper;
    parser->pos--;
    return JR_OK;
}

static int parse_string(struct jr_parser *parser, int len, const char *js,
                        int nnodes, struct jr_node *nodes)
{
    struct jr_node *token;

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
                return JR_OK;
            }
            token = __jr_node_alloc(parser, nnodes, nodes);
            if (token == NULL)
            {
                parser->pos = start;
                return JR_NOMEM;
            }
            fill_node(token, JR_STRING, start + 1, parser->pos);
            token->parent = parser->toksuper;
            return JR_OK;
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
                        return JR_INVAL;
                    }
                    parser->pos++;
                }
                parser->pos--;
                break;
            /* Unexpected symbol */
            default:
                parser->pos = start;
                return JR_INVAL;
            }
        }
    }
    parser->pos = start;
    return JR_INVAL;
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
        return JR_NUMBER;
    case 't':
    case 'f':
        return JR_BOOL;
        break;
    case 'n':
        return JR_NULL;
        break;
    default:
        assert(false);
    }
    assert(false);
    return 0;
}

static void fill_node(struct jr_node *token, const int type, const int start,
                      const int end)
{
    token->type = type;
    token->start = start;
    token->end = end;
    token->size = 0;
}

static int open_bracket(char c, struct jr_parser *parser, int nnodes,
                        struct jr_node *nodes)
{
    struct jr_node *node = __jr_node_alloc(parser, nnodes, nodes);
    if (node == NULL) return JR_NOMEM;
    if (parser->toksuper != -1)
    {
        struct jr_node *t = &nodes[parser->toksuper];
        /* In strict mode an object or array can't become a key */
        if (t->type == JR_OBJECT)
        {
            return JR_INVAL;
        }
        t->size++;
        node->parent = parser->toksuper;
    }
    node->type = (c == '{' ? JR_OBJECT : JR_ARRAY);
    node->start = parser->pos;
    parser->toksuper = parser->toknext - 1;
    return JR_OK;
}

static int check_umatched(struct jr_parser *parser, struct jr_node *nodes)
{
    for (int i = parser->toknext - 1; i >= 0; i--)
    {
        /* Unmatched opened object or array */
        if (nodes[i].start != -1 && nodes[i].end == -1) return JR_INVAL;
    }
    return JR_OK;
}

static int close_bracket(char c, struct jr_parser *parser,
                         struct jr_node *nodes)
{
    int type = (c == '}' ? JR_OBJECT : JR_ARRAY);
    if (parser->toknext < 1)
    {
        return JR_INVAL;
    }
    struct jr_node *node = &nodes[parser->toknext - 1];
    for (;;)
    {
        if (node->start != -1 && node->end == -1)
        {
            if (node->type != type)
            {
                return JR_INVAL;
            }
            node->end = parser->pos + 1;
            parser->toksuper = node->parent;
            break;
        }
        if (node->parent == -1)
        {
            if (node->type != type || parser->toksuper == -1)
            {
                return JR_INVAL;
            }
            break;
        }
        node = &nodes[node->parent];
    }
    return JR_OK;
}
/* meld-cut-here */
