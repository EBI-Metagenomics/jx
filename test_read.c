#include "jx.h"
#include "test_utils.h"
#include <errno.h>
#include <string.h>

JX_DECLARE(jx, 8);

static char json[] = "{ \"name\" : \"Jack\", \"age\" : 27 }";

#define PERSON_NAME_SIZE 32

static struct
{
    char name[PERSON_NAME_SIZE];
    int age;
} person = {0};

int main(void)
{
    jx_init(jx, 8);
    int rc = jx_parse(jx, json);
    ASSERT(rc == 5);
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
    ASSERT(jx_error() == EINVAL);
    jx_clear();
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
    ASSERT(jx_int64_of(jx, "age") == 27);
    return 0;
}
