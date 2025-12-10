#include <termios.h>  // Управление терминалом (struct termios, константы)
#include <unistd.h>   // Системные вызовы ввода-вывода (read, write)
#include <stdio.h>    // Стандартный ввод-вывод (printf, putchar - если нужно, но лучше write)
#include <stdlib.h>   // exit, atexit
#include <ctype.h>    // Проверка типов символов (isprint, isspace)
#include <string.h>   // Работа со строками (для буфера)

// struct line
// {
//     char buffer[41];
//     char len;
// }

const char ctrl_d = 0x04;
const char ctrl_g = 0x07;
const char ctrl_w = 0x17;
const char endl = '\n';
const char *backsp = "\b \b";

struct termios term;
struct termios term_bak;
int restore_bak = 0;

char buf[41];
int pos = 0;
char stack[40];
int s_pos = 0;
int last_wd_len = 0;

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
    term.c_lflag &= ~(ECHO|ICANON);
    tcsetattr(fd, TCSAFLUSH, &term);
}

void write_printable(char c)
{
    buf[pos++] = c;
    buf[pos] = '\0';
    if (c == ' ')
    {
        last_wd_len = 0;
    }
    else
    {
        last_wd_len++;
    }
    write(STDOUT_FILENO, &c, 1);
}

int main(int argc, char const *argv[])
{
    init_term(STDIN_FILENO);
    atexit(restore_term);

    
    while (1)
    {
        char rc;
        read(STDIN_FILENO, &rc, 1);

        if (pos == 0 && rc == ctrl_d) break;
        else if (rc == term.c_cc[VERASE])
        {
            if (pos > 0)
            {
                buf[--pos] = '\0';
                write(STDOUT_FILENO, backsp, 3);
                last_wd_len--;
            }
        }
        else if (rc == term.c_cc[VKILL])
        {
            while (pos > 0)
            {
                buf[--pos] = '\0';
                write(STDOUT_FILENO, backsp, 3);
            }
            last_wd_len = 0;
        }
        else if (rc == ctrl_w)
        {
            while (pos > 0 && buf[pos-1] == ' ')
            {
                buf[--pos] = '\0';
                write(STDOUT_FILENO, backsp, 3);
            }
            while (pos > 0 && buf[pos-1] != ' ')
            {
                buf[--pos] = '\0';
                write(STDOUT_FILENO, backsp, 3);
            }
            last_wd_len = 0;
        }
        else if (rc == '\033')
        {
            write(STDOUT_FILENO, &ctrl_g, 1);
            read(STDIN_FILENO, &rc, 1);
            if (rc == '[')
            {
                read(STDIN_FILENO, &rc, 1);
                if (rc >= '0' && rc <= '9')
                {
                    read(STDIN_FILENO, &rc, 1);
                }
            }
            else
            {
                write_printable(rc);
            }
        }
        else if (rc == '\n')
        {
            write(STDOUT_FILENO, &endl, 1);
            pos = 0;
        }
        else if (isprint(rc))
        {
            if (pos > 39)
            {
                if (last_wd_len < 40)
                {
                    while (pos > 0 && buf[pos - 1] != ' ')
                    {
                        stack[s_pos++] = buf[--pos];
                        write(STDOUT_FILENO, backsp, 3);
                    }
                    write(STDOUT_FILENO, &endl, 1);
                    pos = 0;
                    while (s_pos > 0)
                    {
                        write(STDOUT_FILENO, &stack[s_pos - 1], 1);
                        buf[pos++] = stack[--s_pos];
                    }
                    buf[pos++] = rc;
                    buf[pos] = '\0';
                    last_wd_len++;
                    write(STDOUT_FILENO, &rc, 1);
                }
                else
                {
                    write(STDOUT_FILENO, &ctrl_g, 1);
                }
            }
            else
            {
                write_printable(rc);
            }
        }
        else
        {
            write(STDOUT_FILENO, &ctrl_g, 1);
        }
    }
    
    return 0;
}