#ifndef PRIVATE_H
#define PRIVATE_H

#include "libs/zc_strto_static.h"

#ifndef JSMN_STATIC
#undef JSMN_STATIC
#endif

#include "libs/jsmn.h"

struct jx_item
{
    jsmntype_t type;
    int start;
    int end;
    int size;
#ifdef JSMN_PARENT_LINKS
    int parent;
#endif
};

struct jx
{
    char *str;
    int first_errno;
    jsmn_parser parser;
    unsigned size;
    unsigned size_max;
    struct jsmntok toks[];
};

union jx_union
{
    struct jx_item item;
    struct jsmntok tok;
};

enum
{
    JX_UNDEFINED = 0,
    JX_OBJECT = 1,
    JX_ARRAY = 2,
    JX_STRING = 3,
    JX_NULL = 4,
    JX_BOOL = 5,
    JX_NUMBER = 6,
};

#endif
