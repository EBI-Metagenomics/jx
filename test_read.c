#include "jx_read.h"
#include "jx_write.h"
#include "test_utils.h"
#include <errno.h>
#include <string.h>

JX_DECLARE(jx, 128);

static char person_json[] = "{ \"name\" : \"Jack\", \"age\" : 27 }";
static char unmatched_json[] = "{ \"name\" : \"Jack\", \"age\" : 27 ";
static char array_json[] = "[0, 3, { \"name\" : \"Jack\", \"age\" : 27 }]";
static char empty_array_json[] = "[]";
static char empty_object_json[] = "{}";
static char array_array_json[] = "[[true, false], [null, -23], [-1.0]]";
static char array_string_json[] = "[\"true\"]";

#define PERSON_NAME_SIZE 32

static struct
{
    char name[PERSON_NAME_SIZE];
    int age;
} person = {0};

static void test_person(void);
static void test_unmatched(void);
static void test_array(void);
static void test_empty_array(void);
static void test_object_array(void);
static void test_array_array(void);
static void test_array_string(void);

int main(void)
{
    test_person();
    test_unmatched();
    test_array();
    test_empty_array();
    test_object_array();
    test_array_array();
    test_array_string();
    return 0;
}

static void test_person(void)
{
    JX_INIT(jx);
    ASSERT(!jx_parse(jx, person_json));
    ASSERT(jx_error() == JX_OK);

    ASSERT(jx_type(jx) == JX_OBJECT);
    ASSERT(jx_nchild(jx) == 2);
    ASSERT(jx_type(jx_next(jx)) == JX_STRING);
    ASSERT(jx_nchild(jx) == 1);
    ASSERT(jx_type(jx_next(jx)) == JX_STRING);
    ASSERT(jx_nchild(jx) == 0);
    ASSERT(jx_type(jx_next(jx)) == JX_STRING);
    ASSERT(jx_type(jx_next(jx)) == JX_NUMBER);
    ASSERT(jx_type(jx_next(jx)) == JX_SENTINEL);
    ASSERT(jx_type(jx_next(jx)) == JX_SENTINEL);
    ASSERT(jx_nchild(jx) == 0);

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
    ASSERT(jx_error() == JX_OK);
    ASSERT(jx_type(jx_object_at(jx, "notfound")) == JX_OBJECT);
    ASSERT(jx_error() == JX_NOTFOUND);
    jx_reset(jx);
    ASSERT(jx_type(jx) == JX_OBJECT);
    ASSERT(!strcmp(jx_as_string(jx_object_at(jx, "name")), "Jack"));
    ASSERT(jx_error() == JX_OK);
    jx_up(jx);
    jx_up(jx);
    ASSERT(jx_type(jx) == JX_OBJECT);
    ASSERT(!strcmp(jx_as_string(jx_object_at(jx, "name")), "Jack"));
    ASSERT(jx_error() == JX_OK);
    jx_up(jx);
    jx_up(jx);
    ASSERT(jx_type(jx) == JX_OBJECT);
    ASSERT(jx_as_int(jx_object_at(jx, "age")) == 27);
    ASSERT(jx_error() == JX_OK);
    jx_up(jx);
    jx_up(jx);
    ASSERT(jx_type(jx) == JX_OBJECT);
    ASSERT(!strcmp(jx_string_of(jx, "name"), "Jack"));
    ASSERT(jx_int_of(jx, "age") == 27);
    ASSERT(jx_long_of(jx, "age") == 27);
    ASSERT(jx_uint_of(jx, "age") == 27);
    ASSERT(jx_ulong_of(jx, "age") == 27);
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
    ASSERT(jx_nchild(jx) == 3);
    ASSERT(jx_type(jx_next(jx)) == JX_NUMBER);
    ASSERT(jx_type(jx_next(jx)) == JX_NUMBER);
    ASSERT(jx_type(jx_next(jx)) == JX_OBJECT);
    ASSERT(jx_nchild(jx) == 2);
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

static void test_empty_array(void)
{
    JX_INIT(jx);
    ASSERT(!jx_parse(jx, empty_array_json));

    ASSERT(jx_type(jx) == JX_ARRAY);
    ASSERT(jx_nchild(jx) == 0);
}

static void test_object_array(void)
{
    JX_INIT(jx);
    ASSERT(!jx_parse(jx, empty_object_json));

    ASSERT(jx_type(jx) == JX_OBJECT);
    ASSERT(jx_nchild(jx) == 0);
}

static void test_array_array(void)
{
    JX_INIT(jx);
    ASSERT(!jx_parse(jx, array_array_json));
    ASSERT(jx_error() == JX_OK);

    ASSERT(jx_type(jx) == JX_ARRAY);
    ASSERT(jx_nchild(jx) == 3);
    ASSERT(jx_type(jx_down(jx)) == JX_ARRAY);
    ASSERT(jx_nchild(jx) == 2);
    ASSERT(jx_type(jx_down(jx)) == JX_BOOL);
    ASSERT(jx_as_bool(jx) == true);
    ASSERT(jx_type(jx_next(jx)) == JX_BOOL);
    ASSERT(jx_as_bool(jx) == false);
    ASSERT(jx_type(jx_up(jx)) == JX_ARRAY);
    ASSERT(jx_type(jx_up(jx)) == JX_ARRAY);
    ASSERT(jx_type(jx_array_at(jx, 1)) == JX_ARRAY);
    ASSERT(jx_type(jx_down(jx)) == JX_NULL);
    ASSERT(jx_as_null(jx) == NULL);
    ASSERT(jx_type(jx_next(jx)) == JX_NUMBER);
    ASSERT(jx_as_int(jx) == -23);
    ASSERT(jx_type(jx_up(jx)) == JX_ARRAY);
    ASSERT(jx_type(jx_up(jx)) == JX_ARRAY);
    ASSERT(jx_type(jx_array_at(jx, 2)) == JX_ARRAY);
    ASSERT(jx_type(jx_down(jx)) == JX_NUMBER);
    ASSERT(jx_as_double(jx) == -1.0);
    ASSERT(jx_error() == JX_OK);
}

static void test_array_string(void)
{
    JX_INIT(jx);
    ASSERT(!jx_parse(jx, array_string_json));
    ASSERT(jx_error() == JX_OK);

    ASSERT(jx_type(jx) == JX_ARRAY);
    ASSERT(jx_type(jx_next(jx)) == JX_STRING);
    ASSERT(!strcmp(jx_as_string(jx), "true"));
    ASSERT(jx_as_null(jx) == NULL);
    ASSERT(jx_error() == JX_INVAL);
}
