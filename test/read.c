#include "jr.h"
#include "utils.h"
#include <errno.h>
#include <string.h>

JR_DECLARE(jr, 128);

static char person_json[] = "{ \"name\" : \"Jack\", \"age\" : 27 }";
static char unmatched_json[] = "{ \"name\" : \"Jack\", \"age\" : 27 ";
static char array_json[] = "[0, 3, { \"name\" : \"Jack\", \"age\" : 27 }]";
static char empty_array_json[] = "[]";
static char empty_object_json[] = "{}";
static char array_array_json[] = "[[true, false], [null, -23], [-1.0]]";
static char array_string_json[] = "[\"true\"]";

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
    JR_INIT(jr);
    ASSERT(!jr_parse(jr, person_json));
    ASSERT(jr_error() == JR_OK);

    ASSERT(jr_type(jr) == JR_OBJECT);
    ASSERT(jr_nchild(jr) == 2);
    ASSERT(jr_type(jr_next(jr)) == JR_STRING);
    ASSERT(jr_nchild(jr) == 1);
    ASSERT(jr_type(jr_next(jr)) == JR_STRING);
    ASSERT(jr_nchild(jr) == 0);
    ASSERT(jr_type(jr_next(jr)) == JR_STRING);
    ASSERT(jr_type(jr_next(jr)) == JR_NUMBER);
    ASSERT(jr_type(jr_next(jr)) == JR_SENTINEL);
    ASSERT(jr_type(jr_next(jr)) == JR_SENTINEL);
    ASSERT(jr_nchild(jr) == 0);

    ASSERT(jr_type(jr_back(jr)) == JR_NUMBER);
    ASSERT(jr_type(jr_back(jr)) == JR_STRING);
    ASSERT(jr_type(jr_back(jr)) == JR_STRING);
    ASSERT(jr_type(jr_back(jr)) == JR_STRING);
    ASSERT(jr_type(jr_back(jr)) == JR_OBJECT);
    ASSERT(jr_type(jr_back(jr)) == JR_OBJECT);

    ASSERT(jr_type(jr_up(jr)) == JR_SENTINEL);
    ASSERT(jr_type(jr_back(jr)) == JR_OBJECT);
    ASSERT(jr_type(jr_down(jr)) == JR_STRING);
    ASSERT(jr_type(jr_down(jr)) == JR_STRING);
    ASSERT(jr_type(jr_down(jr)) == JR_SENTINEL);
    ASSERT(jr_type(jr_down(jr)) == JR_SENTINEL);
    ASSERT(jr_type(jr_back(jr)) == JR_STRING);
    ASSERT(jr_type(jr_back(jr)) == JR_STRING);
    ASSERT(jr_type(jr_back(jr)) == JR_OBJECT);

    ASSERT(jr_type(jr_right(jr)) == JR_SENTINEL);
    ASSERT(jr_type(jr_back(jr)) == JR_OBJECT);
    ASSERT(jr_type(jr_down(jr)) == JR_STRING);
    ASSERT(jr_type(jr_right(jr)) == JR_STRING);
    ASSERT(jr_type(jr_right(jr)) == JR_SENTINEL);
    ASSERT(jr_type(jr_back(jr)) == JR_STRING);
    ASSERT(jr_type(jr_up(jr)) == JR_OBJECT);
    ASSERT(jr_error() == JR_OK);
    ASSERT(jr_type(jr_object_at(jr, "notfound")) == JR_OBJECT);
    ASSERT(jr_error() == JR_NOTFOUND);
    jr_reset(jr);
    ASSERT(jr_type(jr) == JR_OBJECT);
    ASSERT(!strcmp(jr_as_string(jr_object_at(jr, "name")), "Jack"));
    ASSERT(jr_error() == JR_OK);
    jr_up(jr);
    jr_up(jr);
    ASSERT(jr_type(jr) == JR_OBJECT);
    ASSERT(!strcmp(jr_as_string(jr_object_at(jr, "name")), "Jack"));
    ASSERT(jr_error() == JR_OK);
    jr_up(jr);
    jr_up(jr);
    ASSERT(jr_type(jr) == JR_OBJECT);
    ASSERT(jr_as_long(jr_object_at(jr, "age")) == 27);
    ASSERT(jr_error() == JR_OK);
    jr_up(jr);
    jr_up(jr);
    ASSERT(jr_type(jr) == JR_OBJECT);
    ASSERT(!strcmp(jr_string_of(jr, "name"), "Jack"));
    ASSERT(jr_long_of(jr, "age") == 27);
    ASSERT(jr_long_of(jr, "age") == 27);
    ASSERT(jr_ulong_of(jr, "age") == 27);
    ASSERT(jr_ulong_of(jr, "age") == 27);
}

static void test_unmatched(void)
{
    JR_INIT(jr);
    ASSERT(jr_parse(jr, unmatched_json) == JR_INVAL);
}

static void test_array(void)
{
    JR_INIT(jr);
    ASSERT(!jr_parse(jr, array_json));

    ASSERT(jr_type(jr) == JR_ARRAY);
    ASSERT(jr_nchild(jr) == 3);
    ASSERT(jr_type(jr_next(jr)) == JR_NUMBER);
    ASSERT(jr_type(jr_next(jr)) == JR_NUMBER);
    ASSERT(jr_type(jr_next(jr)) == JR_OBJECT);
    ASSERT(jr_nchild(jr) == 2);
    ASSERT(jr_type(jr_back(jr)) == JR_NUMBER);
    ASSERT(jr_type(jr_back(jr)) == JR_NUMBER);
    ASSERT(jr_type(jr_back(jr)) == JR_ARRAY);

    ASSERT(jr_type(jr_array_at(jr, 0)) == JR_NUMBER);
    ASSERT(jr_error() == JR_OK);
    ASSERT(jr_type(jr_up(jr)) == JR_ARRAY);

    ASSERT(jr_type(jr_array_at(jr, 1)) == JR_NUMBER);
    ASSERT(jr_error() == JR_OK);
    ASSERT(jr_type(jr_up(jr)) == JR_ARRAY);

    ASSERT(jr_type(jr_array_at(jr, 2)) == JR_OBJECT);
    ASSERT(jr_error() == JR_OK);
    ASSERT(jr_type(jr_up(jr)) == JR_ARRAY);

    ASSERT(jr_type(jr_array_at(jr, 3)) == JR_ARRAY);
    ASSERT(jr_error() == JR_OUTRANGE);
}

static void test_empty_array(void)
{
    JR_INIT(jr);
    ASSERT(!jr_parse(jr, empty_array_json));

    ASSERT(jr_type(jr) == JR_ARRAY);
    ASSERT(jr_nchild(jr) == 0);
}

static void test_object_array(void)
{
    JR_INIT(jr);
    ASSERT(!jr_parse(jr, empty_object_json));

    ASSERT(jr_type(jr) == JR_OBJECT);
    ASSERT(jr_nchild(jr) == 0);
}

static void test_array_array(void)
{
    JR_INIT(jr);
    ASSERT(!jr_parse(jr, array_array_json));
    ASSERT(jr_error() == JR_OK);

    ASSERT(jr_type(jr) == JR_ARRAY);
    ASSERT(jr_nchild(jr) == 3);
    ASSERT(jr_type(jr_down(jr)) == JR_ARRAY);
    ASSERT(jr_nchild(jr) == 2);
    ASSERT(jr_type(jr_down(jr)) == JR_BOOL);
    ASSERT(jr_as_bool(jr) == true);
    ASSERT(jr_type(jr_next(jr)) == JR_BOOL);
    ASSERT(jr_as_bool(jr) == false);
    ASSERT(jr_type(jr_up(jr)) == JR_ARRAY);
    ASSERT(jr_type(jr_up(jr)) == JR_ARRAY);
    ASSERT(jr_type(jr_array_at(jr, 1)) == JR_ARRAY);
    ASSERT(jr_type(jr_down(jr)) == JR_NULL);
    ASSERT(jr_as_null(jr) == NULL);
    ASSERT(jr_type(jr_next(jr)) == JR_NUMBER);
    ASSERT(jr_as_long(jr) == -23);
    ASSERT(jr_type(jr_up(jr)) == JR_ARRAY);
    ASSERT(jr_type(jr_up(jr)) == JR_ARRAY);
    ASSERT(jr_type(jr_array_at(jr, 2)) == JR_ARRAY);
    ASSERT(jr_type(jr_down(jr)) == JR_NUMBER);
    ASSERT(jr_as_double(jr) == -1.0);
    ASSERT(jr_error() == JR_OK);
}

static void test_array_string(void)
{
    JR_INIT(jr);
    ASSERT(!jr_parse(jr, array_string_json));
    ASSERT(jr_error() == JR_OK);

    ASSERT(jr_type(jr) == JR_ARRAY);
    ASSERT(jr_type(jr_next(jr)) == JR_STRING);
    ASSERT(!strcmp(jr_as_string(jr), "true"));
    ASSERT(jr_as_null(jr) == NULL);
    ASSERT(jr_error() == JR_INVAL);
}
