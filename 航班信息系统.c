#include "client.h"
#include "menu.h"
#include "server.h"

#include <string.h>

int main(int argc, char* argv[])
{
    if (argc >= 2)
    {
        if (strcmp(argv[1], "server") == 0)
        {
            runServer();
            return 0;
        }
        if (strcmp(argv[1], "local") == 0)
        {
            runSystem();
            return 0;
        }
    }
    runClient();
    return 0;
}
