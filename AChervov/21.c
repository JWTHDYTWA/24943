#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

struct termios term;
struct termios term_bak;
int restore_bak = 0;

void restore_term()
{
    if (restore_bak)
    {
        tcsetattr(0, TCSAFLUSH, &term_bak);
    }
}

void init_term(int fd)
{
    tcgetattr(fd, &term);
    term_bak = term;
    restore_bak = 1;
    atexit(restore_term);
    term.c_lflag &= ~(ECHO|ICANON);
    tcsetattr(fd, TCSAFLUSH, &term);
}

int main(int argc, char const *argv[])
{
    init_term(0);
    return 0;
}
