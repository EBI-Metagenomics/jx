#ifndef JR_PARSER_H
#define JR_PARSER_H

#include <stddef.h>

/* meld-cut-here */
struct jr_parser
{
    int alloc_size;
    int size;
    int pos;
    int toknext;
    int toksuper;
};
/* meld-cut-here */

#endif
