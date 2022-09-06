#include "jx.h"
#include "test_utils.h"
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
#if 0
    ASSERT(jx_errno(&jx) == 0);
    ASSERT(jx_type(it) == JX_OBJECT);

    it = jx_right(&jx, it);
    ASSERT(jx_type(it) == JX_UNDEF);
    it = jx_back(&jx, it);
    ASSERT(jx_type(it) == JX_OBJECT);

    it = jx_down(&jx, it);
    ASSERT(jx_type(it) == JX_STRING);
    it = jx_down(&jx, it);
    ASSERT(jx_type(it) == JX_STRING);
    it = jx_down(&jx, it);
    ASSERT(jx_type(it) == JX_UNDEF);
    it = jx_down(&jx, it);
    ASSERT(jx_type(it) == JX_UNDEF);
    it = jx_back(&jx, it);
    ASSERT(jx_type(it) == JX_STRING);
    struct jx_string *str = jx_as_string(&jx, it);
    char *dup = jx_strdup(&jx, str);
    ASSERT(dup != NULL);
    ASSERT(jx_errno(&jx) == 0);
    ASSERT(strcmp(dup, "Jack") == 0);
    free(dup);
    jx_strcpy(&jx, person.name, str, PERSON_NAME_SIZE);
    ASSERT(jx_errno(&jx) == 0);
    ASSERT(strcmp(person.name, "Jack") == 0);

    it = jx_up(&jx, it);
    ASSERT(jx_type(it) == JX_STRING);

    it = jx_right(&jx, it);
    ASSERT(jx_type(it) == JX_STRING);
    ASSERT(jx_errno(&jx) == 0);
    dup = jx_strdup(&jx, jx_as_string(&jx, it));
    ASSERT(strcmp(dup, "age") == 0);
    free(dup);

    it = jx_down(&jx, it);
    ASSERT(jx_type(it) == JX_NUMBER);
#endif
    return 0;
}
