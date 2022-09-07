#include "jx_node.h"
#include "jx_compiler.h"
#include "jx_parser.h"

struct jx_node *__jx_node_alloc(struct jx_parser *parser, int nnodes,
                                struct jx_node *nodes)
{
    if (parser->toknext >= nnodes) return NULL;
    struct jx_node *node = &nodes[parser->toknext++];
    node->start = -1;
    node->end = -1;
    node->size = 0;
    node->parent = -1;
    return node;
}
