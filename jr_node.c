#include "jr_node.h"
#include "jr_compiler.h"
#include "jr_parser.h"

/* meld-cut-here */
struct jr_node *__jr_node_alloc(struct jr_parser *parser, int nnodes,
                                struct jr_node *nodes)
{
    if (parser->toknext >= nnodes) return NULL;
    struct jr_node *node = &nodes[parser->toknext++];
    node->start = -1;
    node->end = -1;
    node->size = 0;
    node->parent = -1;
    return node;
}
/* meld-cut-here */
