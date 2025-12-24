#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SOC_NAME "socket"

int main(int argc, char const *argv[])
{
    int soc = socket(AF_UNIX, SOCK_STREAM, 0);
    if (soc < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOC_NAME, sizeof(addr.sun_path) - 1);

    if (connect(soc, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("Connect failed");
        close(soc);
        return 1;
    }

    printf("Connected. Type text and press Enter (Ctrl+D to exit).\n");

    char buf[1024];
    while (fgets(buf, sizeof(buf), stdin) != NULL)
    {
        ssize_t sent = write(soc, buf, strlen(buf));
        if (sent < 0)
        {
            perror("Write failed");
            break;
        }
    }

    close(soc);
    return 0;
}
