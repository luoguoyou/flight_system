#include "utils.h"

#include "common.h"
#include <clocale>
#include <windows.h>

void initConsole()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    setlocale(LC_ALL, ".UTF-8");
}

void stripBom(char text[])
{
    if ((unsigned char)text[0] == 0xEF &&
        (unsigned char)text[1] == 0xBB &&
        (unsigned char)text[2] == 0xBF)
    {
        memmove(text, text + 3, strlen(text + 3) + 1);
    }
}
