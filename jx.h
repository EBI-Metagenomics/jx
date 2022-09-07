#ifndef JX_H
#define JX_H

#include "jx_cursor.h"
#include "jx_error.h"
#include "jx_node.h"
#include "jx_parser.h"
#include "jx_type.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct jx
{
    union
    {
        struct jx_parser parser;
        struct jx_cursor cursor;
        struct jx_node node;
    };
};

#define __JX_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define JX_DECLARE(name, size) struct jx name[size];
#define JX_INIT(name) __jx_init((name), __JX_ARRAY_SIZE(name))

void __jx_init(struct jx[], int size);
int jx_parse(struct jx[], char *json);
int jx_error(void);
void jx_reset(struct jx[]);
int jx_type(struct jx const[]);

struct jx *jx_back(struct jx[]);
struct jx *jx_down(struct jx[]);
struct jx *jx_next(struct jx[]);
struct jx *jx_right(struct jx[]);
struct jx *jx_up(struct jx[]);

struct jx *jx_array_at(struct jx[], int idx);
struct jx *jx_object_at(struct jx[], char const *key);

char *jx_string_of(struct jx[], char const *key);
long jx_long_of(struct jx[], char const *key);
int jx_int_of(struct jx[], char const *key);
unsigned long jx_ulong_of(struct jx[], char const *key);
unsigned int jx_uint_of(struct jx[], char const *key);

char *jx_as_string(struct jx[]);
long jx_as_long(struct jx[]);
int jx_as_int(struct jx[]);
unsigned long jx_as_ulong(struct jx[]);
unsigned int jx_as_uint(struct jx[]);

#endif
