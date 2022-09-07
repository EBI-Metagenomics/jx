#ifndef JR_NODE_H
#define JR_NODE_H

/* meld-cut-here */
struct jr_node
{
    int type;
    int start;
    int end;
    int size;
    int parent;
    int prev;
};
/* meld-cut-here */

struct jr_parser;

struct jr_node *__jr_node_alloc(struct jr_parser *parser, int nnodes,
                                struct jr_node *nodes);

#endif
