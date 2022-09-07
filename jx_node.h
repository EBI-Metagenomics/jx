#ifndef JX_NODE_H
#define JX_NODE_H

struct jx_node
{
    int type;
    int start;
    int end;
    int size;
    int parent;
    int prev;
};

struct jx_parser;

struct jx_node *node_alloc(struct jx_parser *parser, int nnodes,
                           struct jx_node *nodes);

#endif
