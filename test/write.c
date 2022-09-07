#include "jw.h"
#include "utils.h"
#include <errno.h>
#include <limits.h>
#include <string.h>

static char json[1024] = {0};
static char desired[1024] = {0};

int main(void)
{
    char *js = json;
    js += jw_array_open(js);
    js += jw_long(js, LONG_MIN);
    js += jw_comma(js);
    js += jw_long(js, LONG_MAX);
    js += jw_comma(js);
    js += jw_ulong(js, 0);
    js += jw_comma(js);
    js += jw_ulong(js, ULONG_MAX);
    js += jw_comma(js);
    js += jw_string(js, "hello");
    js += jw_comma(js);
    js += jw_null(js);
    js += jw_comma(js);
    js += jw_bool(js, true);
    js += jw_comma(js);
    js += jw_bool(js, false);
    js += jw_comma(js);
    js += jw_object_open(js);
    js += jw_string(js, "name");
    js += jw_colon(js);
    js += jw_string(js, "danilo");
    js += jw_object_close(js);
    js += jw_array_close(js);
    *js = '\0';
    sprintf(desired,
            "[%ld,%ld,%lu,%lu,\"hello\",null,true,false,{\"name\":\"danilo\"}]",
            LONG_MIN, LONG_MAX, 0UL, ULONG_MAX);
    ASSERT(!strcmp(json, desired));
    return 0;
}
