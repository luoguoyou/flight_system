#include "utils.h"

#include "common.h"

void stripBom(char text[])
{
    if ((unsigned char)text[0] == 0xEF &&
        (unsigned char)text[1] == 0xBB &&
        (unsigned char)text[2] == 0xBF)
    {
        memmove(text, text + 3, strlen(text + 3) + 1);
    }
}
