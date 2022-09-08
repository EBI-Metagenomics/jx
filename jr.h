#ifndef JR_H
#define JR_H

#include "jr_cursor.h"
#include "jr_error.h"
#include "jr_node.h"
#include "jr_parser.h"
#include "jr_type.h"

/* meld-cut-here */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct jr
{
    union
    {
        struct jr_parser parser;
        struct jr_cursor cursor;
        struct jr_node node;
    };
};

#define __JR_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define JR_DECLARE(name, size) struct jr name[size];
#define JR_INIT(name) __jr_init((name), __JR_ARRAY_SIZE(name))

void __jr_init(struct jr[], int alloc_size);
int jr_parse(struct jr[], int length, char *json);
int jr_error(void);
void jr_reset(struct jr[]);
int jr_type(struct jr const[]);
int jr_nchild(struct jr const[]);

struct jr *jr_back(struct jr[]);
struct jr *jr_down(struct jr[]);
struct jr *jr_next(struct jr[]);
struct jr *jr_right(struct jr[]);
struct jr *jr_up(struct jr[]);

struct jr *jr_array_at(struct jr[], int idx);
struct jr *jr_object_at(struct jr[], char const *key);

char *jr_string_of(struct jr[], char const *key);
void jr_strcpy_of(struct jr[], char const *key, char *dst, int size);
bool jr_bool_of(struct jr[], char const *key);
void *jr_null_of(struct jr[], char const *key);
long jr_long_of(struct jr[], char const *key);
unsigned long jr_ulong_of(struct jr[], char const *key);
double jr_double_of(struct jr[], char const *key);

char *jr_as_string(struct jr[]);
bool jr_as_bool(struct jr[]);
void *jr_as_null(struct jr[]);
long jr_as_long(struct jr[]);
unsigned long jr_as_ulong(struct jr[]);
double jr_as_double(struct jr[]);
/* meld-cut-here */

#endif
