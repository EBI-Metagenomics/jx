#include "jx_cursor.h"
#include <string.h>

void __jx_cursor_init(struct jx_cursor *cursor, char *json)
{
    cursor->length = strlen(json);
    cursor->json = json;
    cursor->pos = 0;
}
