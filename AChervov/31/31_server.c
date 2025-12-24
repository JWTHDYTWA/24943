#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>

#define SOC_NAME "socket"

int serv_sock;
sig_atomic_t stop = 0;

void assert_noerr(int retcode, const char *msg)
{
    if (retcode < 0)
    {
        fprintf(stderr, "%s\nError: %s\n", msg, strerror(errno));
        exit(1);
    }
}

void exit_handler()
{
    close(serv_sock);
    unlink(SOC_NAME);
}

int main(int argc, char const *argv[])
{
    atexit(exit_handler);
    signal(SIGINT, exit);

    int ret;
    serv_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    assert_noerr(serv_sock, "Couldn't create a socket.");

    struct sockaddr_un addr;
    socklen_t addr_s = sizeof(addr);
    memset(&addr, 0, addr_s);
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOC_NAME, sizeof(addr.sun_path)-1);

    ret = bind(serv_sock, (struct sockaddr*)&addr, addr_s);
    assert_noerr(ret, "Couldn't bind socket to a local file. Probably it already exists.");
    ret = listen(serv_sock, 5);
    assert_noerr(ret, "`listen` error.\n");

    struct pollfd fds[10];
    fds[0].fd = serv_sock;
    fds[0].events = POLLIN;
    int fds_cnt = 1;

    char buf;
    while (1)
    {
        int ret = poll(fds, fds_cnt, -1);

        assert_noerr(ret, "Poll error.");

        for (int i = 0; i < fds_cnt; i++)
        {
            if (fds[i].revents == 0)
                continue;

            if (fds[i].revents & POLLIN)
            {
                if (fds[i].fd == serv_sock)
                {
                    if (fds_cnt < 10)
                    {
                        int new_client = accept(serv_sock, (struct sockaddr *)&addr, &addr_s);
                        assert_noerr(new_client, "Accept new client error.");
                        if (new_client >= 0)
                        {
                            fds[fds_cnt].fd = new_client;
                            fds[fds_cnt].events = POLLIN;
                            fds_cnt++;
                        }
                    }
                    else
                    {
                        fprintf(stderr, "Too many clients\n");
                    }
                }
                else
                {
                    ssize_t count = read(fds[i].fd, &buf, 1);
                    if (count <= 0)
                    {
                        close(fds[i].fd);
                        fds[i] = fds[fds_cnt - 1];
                        fds_cnt--;
                        i--;
                    }
                    else
                    {
                        buf = toupper(buf);
                        write(STDOUT_FILENO, &buf, 1);
                    }
                }
            }
        }
    }

    return 0;
}
