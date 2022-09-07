#ifndef JX_PARSER_H
#define JX_PARSER_H

#include <stddef.h>

struct jx_parser
{
    int size;
    int count;
    unsigned pos;
    unsigned toknext;
    int toksuper;
};

struct jx_node;

void parser_init(struct jx_parser *parser, int size);
int parser_parse(struct jx_parser *, size_t length, char *json, int nnodes,
                 struct jx_node *);

#endif
