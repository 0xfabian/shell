#include <terminal.h>

termios original;

void tty_raw()
{
    int ret = 0;

    ret = tcgetattr(fileno(stdin), &original);
    if (ret == -1)
        perror("tty_raw: tcgetattr");

    termios raw = original;

    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 1;

    ret = tcsetattr(fileno(stdin), TCSAFLUSH, &raw);
    if (ret == -1)
        perror("tty_raw: tcsetattr");
}

void tty_restore()
{
    int ret = tcsetattr(fileno(stdin), TCSAFLUSH, &original);
    if (ret == -1)
        perror("tty_restore: tcsetattr");
}

void cursor_move_left(int steps)
{
    if (steps == 0)
        return;

    printf("\e[%dD", steps);
}

void cursor_move_right(int steps)
{
    if (steps == 0)
        return;

    printf("\e[%dC", steps);
}
