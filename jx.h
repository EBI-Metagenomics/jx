#ifndef JX_H
#define JX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum
{
    JX_SENTINEL = 0,
    JX_OBJECT = 1,
    JX_ARRAY = 2,
    JX_STRING = 3,
    JX_NULL = 4,
    JX_BOOL = 5,
    JX_NUMBER = 6,
};

struct jx_parser
{
    int bits;
    int nnodes;
    unsigned pos;
    unsigned toknext;
    int toksuper;
};

struct jx_cursor
{
    int length;
    char *json;
    int pos;
};

struct jx_node
{
    int type;
    int start;
    int end;
    int size;
    int parent;
    int prev;
};

struct jx
{
    union
    {
        struct jx_parser parser;
        struct jx_cursor cursor;
        struct jx_node node;
    };
};

#define JX_DECLARE(name, bits) struct jx name[1 << (bits)];

void jx_init(struct jx[], int bits);
int jx_parse(struct jx[], char *json);

/* --query begin--------------------------------------------------------------*/
int jx_errno(void);
void jx_clear(void);
int jx_type(struct jx const[]);
/* --query end----------------------------------------------------------------*/

/* --navigation begin---------------------------------------------------------*/
struct jx *jx_back(struct jx[]);
struct jx *jx_down(struct jx[]);
struct jx *jx_next(struct jx[]);
struct jx *jx_right(struct jx[]);
struct jx *jx_up(struct jx[]);

struct jx *jx_array_at(struct jx[], int idx);
struct jx *jx_object_at(struct jx[], char const *key);
/* --navigation end-----------------------------------------------------------*/

#if 0
/* --casting begin------------------------------------------------------------*/
struct jx_array *jx_as_array(struct jx *, struct jx_it *);
struct jx_object *jx_as_object(struct jx *, struct jx_it *);
#endif

char *jx_as_string(struct jx[]);
// int64_t jx_as_int64(struct jx *, struct jx_it const *);
// int32_t jx_as_int32(struct jx *, struct jx_it const *);
int jx_as_int(struct jx[]);
#if 0
uint64_t jx_as_uint64(struct jx *, struct jx_it const *);
uint32_t jx_as_uint32(struct jx *, struct jx_it const *);
unsigned jx_as_uint(struct jx *, struct jx_it const *);
double jx_as_double(struct jx *, struct jx_it const *);
float jx_as_float(struct jx *, struct jx_it const *);
bool jx_as_bool(struct jx *, struct jx_it const *);
void *jx_as_null(struct jx *, struct jx_it const *);
/* --casting end--------------------------------------------------------------*/

/* --string utils begin-------------------------------------------------------*/
char *jx_strdup(struct jx *, struct jx_string const *);
void jx_strcpy(struct jx *, char *dst, struct jx_string const *,
               size_t dst_size);
size_t jx_strlen(struct jx *, struct jx_string const *);
/* --string utils end---------------------------------------------------------*/
#endif

#endif
