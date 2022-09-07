#include "jx.h"
#include "test_utils.h"
#include <errno.h>
#include <string.h>

JX_DECLARE(jx, 8);

static char person_json[] = "{ \"name\" : \"Jack\", \"age\" : 27 }";
static char unmatched_json[] = "{ \"name\" : \"Jack\", \"age\" : 27 ";
static char array_json[] = "[0, 3, { \"name\" : \"Jack\", \"age\" : 27 }]";

#define PERSON_NAME_SIZE 32

static struct
{
    char name[PERSON_NAME_SIZE];
    int age;
} person = {0};

static void test_person(void);
static void test_unmatched(void);
static void test_array(void);

int main(void)
{
    test_person();
    test_unmatched();
    test_array();
    return 0;
}

static void test_person(void)
{
    JX_INIT(jx);
    ASSERT(!jx_parse(jx, person_json));
    ASSERT(jx_error() == 0);

    ASSERT(jx_type(jx) == JX_OBJECT);
    ASSERT(jx_type(jx_next(jx)) == JX_STRING);
    ASSERT(jx_type(jx_next(jx)) == JX_STRING);
    ASSERT(jx_type(jx_next(jx)) == JX_STRING);
    ASSERT(jx_type(jx_next(jx)) == JX_NUMBER);
    ASSERT(jx_type(jx_next(jx)) == JX_SENTINEL);
    ASSERT(jx_type(jx_next(jx)) == JX_SENTINEL);

    ASSERT(jx_type(jx_back(jx)) == JX_NUMBER);
    ASSERT(jx_type(jx_back(jx)) == JX_STRING);
    ASSERT(jx_type(jx_back(jx)) == JX_STRING);
    ASSERT(jx_type(jx_back(jx)) == JX_STRING);
    ASSERT(jx_type(jx_back(jx)) == JX_OBJECT);
    ASSERT(jx_type(jx_back(jx)) == JX_OBJECT);

    ASSERT(jx_type(jx_up(jx)) == JX_SENTINEL);
    ASSERT(jx_type(jx_back(jx)) == JX_OBJECT);
    ASSERT(jx_type(jx_down(jx)) == JX_STRING);
    ASSERT(jx_type(jx_down(jx)) == JX_STRING);
    ASSERT(jx_type(jx_down(jx)) == JX_SENTINEL);
    ASSERT(jx_type(jx_down(jx)) == JX_SENTINEL);
    ASSERT(jx_type(jx_back(jx)) == JX_STRING);
    ASSERT(jx_type(jx_back(jx)) == JX_STRING);
    ASSERT(jx_type(jx_back(jx)) == JX_OBJECT);

    ASSERT(jx_type(jx_right(jx)) == JX_SENTINEL);
    ASSERT(jx_type(jx_back(jx)) == JX_OBJECT);
    ASSERT(jx_type(jx_down(jx)) == JX_STRING);
    ASSERT(jx_type(jx_right(jx)) == JX_STRING);
    ASSERT(jx_type(jx_right(jx)) == JX_SENTINEL);
    ASSERT(jx_type(jx_back(jx)) == JX_STRING);
    ASSERT(jx_type(jx_up(jx)) == JX_OBJECT);
    ASSERT(jx_error() == 0);
    ASSERT(jx_type(jx_object_at(jx, "notfound")) == JX_OBJECT);
    ASSERT(jx_error() == JX_NOTFOUND);
    jx_reset(jx);
    ASSERT(jx_type(jx) == JX_OBJECT);
    ASSERT(!strcmp(jx_as_string(jx_object_at(jx, "name")), "Jack"));
    ASSERT(jx_error() == 0);
    jx_up(jx);
    jx_up(jx);
    ASSERT(jx_type(jx) == JX_OBJECT);
    ASSERT(!strcmp(jx_as_string(jx_object_at(jx, "name")), "Jack"));
    ASSERT(jx_error() == 0);
    jx_up(jx);
    jx_up(jx);
    ASSERT(jx_type(jx) == JX_OBJECT);
    ASSERT(jx_as_int(jx_object_at(jx, "age")) == 27);
    ASSERT(jx_error() == 0);
    jx_up(jx);
    jx_up(jx);
    ASSERT(jx_type(jx) == JX_OBJECT);
    ASSERT(!strcmp(jx_string_of(jx, "name"), "Jack"));
    ASSERT(jx_int_of(jx, "age") == 27);
}

static void test_unmatched(void)
{
    JX_INIT(jx);
    ASSERT(jx_parse(jx, unmatched_json) == JX_INVAL);
}

static void test_array(void)
{
    JX_INIT(jx);
    ASSERT(!jx_parse(jx, array_json));

    ASSERT(jx_type(jx) == JX_ARRAY);
    ASSERT(jx_type(jx_next(jx)) == JX_NUMBER);
    ASSERT(jx_type(jx_next(jx)) == JX_NUMBER);
    ASSERT(jx_type(jx_next(jx)) == JX_OBJECT);
    ASSERT(jx_type(jx_back(jx)) == JX_NUMBER);
    ASSERT(jx_type(jx_back(jx)) == JX_NUMBER);
    ASSERT(jx_type(jx_back(jx)) == JX_ARRAY);

    ASSERT(jx_type(jx_array_at(jx, 0)) == JX_NUMBER);
    ASSERT(jx_error() == JX_OK);
    ASSERT(jx_type(jx_up(jx)) == JX_ARRAY);

    ASSERT(jx_type(jx_array_at(jx, 1)) == JX_NUMBER);
    ASSERT(jx_error() == JX_OK);
    ASSERT(jx_type(jx_up(jx)) == JX_ARRAY);

    ASSERT(jx_type(jx_array_at(jx, 2)) == JX_OBJECT);
    ASSERT(jx_error() == JX_OK);
    ASSERT(jx_type(jx_up(jx)) == JX_ARRAY);

    ASSERT(jx_type(jx_array_at(jx, 3)) == JX_ARRAY);
    ASSERT(jx_error() == JX_OUTRANGE);
}
