#ifndef JX_H
#define JX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum
{
    JX_UNDEF = 0,
    JX_OBJECT = 1,
    JX_ARRAY = 2,
    JX_STRING = 3,
    JX_NULL = 4,
    JX_BOOL = 5,
    JX_NUMBER = 6,
};

struct jx_object
{
    int start;
    int end;
    int size;
    int type;
    int parent;
    int previous;
};

struct jx_array
{
    int start;
    int end;
    int size;
    int type;
    int parent;
    int previous;
};

struct jx_string
{
    int start;
    int end;
    int size;
    int type;
    int parent;
    int previous;
};

struct jx_it
{
    int start;
    int end;
    int size;
    int type;
    int parent;
    int previous;
};

struct jx
{
    size_t length;
    char const *json;

    int ferrno;

    struct
    {
        unsigned pos;     /* offset in the JSON string */
        unsigned toknext; /* next token to allocate */
        int toksuper;     /* superior token node, e.g. parent object or array */
    } parser;

    unsigned nitems;
    unsigned max_nitems;
    struct jx_it *it;
};

#define JX_DECLARE(name, bits)                                                 \
    static struct jx_it name##_container[1 << (bits)];                         \
    static struct jx name = {                                                  \
        0, "", 0, {0, 0, -1}, 0, 1 << (bits), &(name##_container[0])};

struct jx_it *jx_parse(struct jx *, char const *json);

/* --query begin--------------------------------------------------------------*/
int jx_errno(struct jx const *);
int jx_type(struct jx_it const *);
/* --query end----------------------------------------------------------------*/

/* --navigation begin---------------------------------------------------------*/
struct jx_it *jx_back(struct jx *, struct jx_it *);
struct jx_it *jx_down(struct jx *, struct jx_it *);
struct jx_it *jx_next(struct jx *, struct jx_it *);
struct jx_it *jx_right(struct jx *, struct jx_it *);
struct jx_it *jx_up(struct jx *, struct jx_it *);

struct jx_it *jx_array_at(struct jx *, struct jx_array *, unsigned idx);
struct jx_it *jx_object_at(struct jx *, struct jx_object *, char const *key);

#define jx_at(jx, it, pos)                                                     \
    _Generic((it), struct jx_array *                                           \
             : jx_array_at, struct jx_object *                                 \
             : jx_object_at)(jx, it, pos)
/* --navigation end-----------------------------------------------------------*/

/* --casting begin------------------------------------------------------------*/
struct jx_array *jx_as_array(struct jx *, struct jx_it *);
struct jx_object *jx_as_object(struct jx *, struct jx_it *);
struct jx_string *jx_as_string(struct jx *, struct jx_it *);
int64_t jx_as_int64(struct jx *, struct jx_it const *);
int32_t jx_as_int32(struct jx *, struct jx_it const *);
int jx_as_int(struct jx *, struct jx_it const *);
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
