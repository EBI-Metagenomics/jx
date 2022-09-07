#include "jr_cursor.h"
/* meld-cut-here */
#include <string.h>

void __jr_cursor_init(struct jr_cursor *cursor, char *json)
{
    cursor->length = strlen(json);
    cursor->json = json;
    cursor->pos = 0;
}
/* meld-cut-here */
