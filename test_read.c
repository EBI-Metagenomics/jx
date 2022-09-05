#include "jx.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JX_NITEMS 128

inline static void print_ctx(char const *func, char const *file, int line)
{
    fprintf(stderr, "\n%s failed at %s:%d\n", func, file, line);
}

#define ASSERT(x)                                                              \
    do                                                                         \
    {                                                                          \
        if (!(x))                                                              \
        {                                                                      \
            print_ctx(__FUNCTION__, __FILE__, __LINE__);                       \
            exit(1);                                                           \
        }                                                                      \
    } while (0);

static char text0[] = "{ \"name\" : \"Jack\", \"age\" : 27 }";
static char text1[] = "[1, 6]";
static char text2[] = "[[1, 4], 6]";
static char text3[] = "[{ \"name\" : \"Jack\"}, 6]";
static char text4[] = "null";
static char text5[] = "{null}";
static char text6[] =
    "{ \"name\" : \"Jack\", \"age\" : 27, \"scores\": [1, 2, 3, 4] }";

struct person
{
    char const *name;
    int age;
};

char const *item_type(struct jx const *jx, struct jx_item const *item)
{
    if (jx_is_array(jx, item)) return "array";
    if (jx_is_bool(jx, item)) return "bool";
    if (jx_is_null(jx, item)) return "null";
    if (jx_is_number(jx, item)) return "number";
    if (jx_is_object(jx, item)) return "object";
    if (jx_is_string(jx, item)) return "string";
    return "";
}

void print_item(struct jx const *jx, struct jx_item const *item)
{
    printf("item:\n");
    printf("    type  : %s\n", item_type(jx, item));
    printf("    size  : %d\n", jx_nchild(jx, item));
}

static void test_flat_object(void);

int main(void)
{
    test_flat_object();
    return 0;
}

static void test_flat_object(void)
{
    static char json[] = "{ \"name\" : \"Jack\", \"age\" : 27 }";
    struct person person = {0};

    struct jx *jx = jx_new(JX_NITEMS);
    ASSERT(jx);
    ASSERT(jx_parse(jx, json) == 5);
    jx_assert_nitems(jx, 5);

    struct jx_item const *item = jx_root(jx);
    jx_assert_object(jx, item);
    item = jx_next(jx, item);
    while (!jx_end(jx, item))
    {
        jx_assert_string(jx, item);
        if (!strcmp(jx_string(jx, item), "name"))
        {
            item = jx_next(jx, item);
            jx_assert_string(jx, item);
            person.name = jx_string(jx, item);
        }
        else if (!strcmp(jx_string(jx, item), "age"))
        {
            item = jx_next(jx, item);
            jx_assert_number(jx, item);
            person.age = jx_int64(jx, item);
        }
        item = jx_next(jx, item);
    }

    ASSERT(!jx_errno(jx));
    ASSERT(!strcmp(person.name, "Jack"));
    ASSERT(person.age == 27);
    jx_del(jx);
}
