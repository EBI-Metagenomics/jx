#ifndef JR_PARSER_H
#define JR_PARSER_H

#include <stddef.h>

/* meld-cut-here */
struct jr_parser
{
    int alloc_size;
    int size;
    unsigned pos;
    unsigned toknext;
    int toksuper;
};
/* meld-cut-here */

struct jr_node;

void __jr_parser_init(struct jr_parser *parser, int size);
int __jr_parser_parse(struct jr_parser *, size_t length, char *json, int nnodes,
                      struct jr_node *);

#endif
