#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/filio.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>

#define SOC_NAME "socket"
#define MAX_CLIENTS 10
#define BUF_SIZE 1

int serv_sock;
int clients[MAX_CLIENTS];
int clients_cnt = 0;

void make_async(int fd)
{
    if (fcntl(fd, F_SETOWN, getpid()) < 0) {
        perror("fcntl F_SETOWN");
        exit(1);
    }

    int flags = fcntl(fd, F_GETFL);
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl F_SETFL");
        exit(1);
    }

    int on = 1;
    if (ioctl(fd, FIOASYNC, &on) < 0) {
        perror("ioctl FIOASYNC");
        exit(1);
    }
}

void cleanup()
{
    close(serv_sock);
    for (int i = 0; i < clients_cnt; i++)
    {
        close(clients[i]);
    }
    unlink(SOC_NAME);
    printf("\nServer shutdown cleanup done.\n");
}

void sigint_handler(int sig)
{
    exit(0);
}

void sigio_handler(int sig)
{
    while (1)
    {
        struct sockaddr_un cli_addr;
        socklen_t len = sizeof(cli_addr);
        int new_fd = accept(serv_sock, (struct sockaddr *)&cli_addr, &len);

        if (new_fd < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                break;
            }
            perror("Accept error");
            break;
        }

        if (clients_cnt < MAX_CLIENTS)
        {
            make_async(new_fd);
            clients[clients_cnt++] = new_fd;
            const char *msg = "[Server] New client connected\n";
            write(STDOUT_FILENO, msg, strlen(msg));
        }
        else
        {
            const char *msg = "[Server] Too many clients, rejected\n";
            write(STDERR_FILENO, msg, strlen(msg));
            close(new_fd);
        }
    }

    for (int i = 0; i < clients_cnt; i++)
    {
        char buf;
        int fd = clients[i];

        while (1)
        {
            ssize_t n = read(fd, &buf, 1);

            if (n > 0)
            {
                buf = toupper(buf);
                write(STDOUT_FILENO, &buf, 1);
            }
            else if (n == 0)
            {
                close(fd);
                clients[i] = clients[clients_cnt - 1];
                clients_cnt--;
                i--;

                const char *msg = "[Server] Client disconnected\n";
                write(STDOUT_FILENO, msg, strlen(msg));
                break;
            }
            else
            {
                if (errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    break;
                }
                break;
            }
        }
    }
}

int main(int argc, char const *argv[])
{
    atexit(cleanup);
    signal(SIGINT, sigint_handler);

    struct sigaction sa;
    sa.sa_handler = sigio_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGIO, &sa, NULL);

    serv_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serv_sock < 0)
    {
        perror("Socket creation error.\n");
        exit(1);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOC_NAME, sizeof(addr.sun_path) - 1);

    if (bind(serv_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind (delete socket file if exists)");
        exit(1);
    }

    if (listen(serv_sock, 5) < 0)
    {
        perror("listen");
        exit(1);
    }

    make_async(serv_sock);

    printf("Server started (Async I/O). Waiting for clients...\n");

    while (1)
    {
        pause();
    }

    return 0;
}