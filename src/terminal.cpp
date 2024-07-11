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

void set_cursor(size_t pos)
{
    struct winsize size;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &size);

    int line = pos / size.ws_col + 1;
    int column = pos % size.ws_col + 1;

    printf("\e[%d;%dH", line, column);
}

size_t get_cursor()
{
    if (write(STDOUT_FILENO, "\e[6n", 4) != 4)
        return 0;

    char buf[32];
    size_t i = 0;

    while (i < sizeof(buf) - 1)
    {
        if (read(STDIN_FILENO, &buf[i], 1) != 1)
            break;

        if (buf[i] == 'R')
            break;

        i++;
    }

    buf[i] = '\0';

    int line, column;

    sscanf(&buf[2], "%d;%d", &line, &column);

    struct winsize size;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &size);

    return size.ws_col * (line - 1) + (column - 1);
}

void ajust_for_scroll(size_t* anchor, size_t offset)
{
    struct winsize size;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &size);

    int line = (*anchor + offset) / size.ws_col + 1;

    if (line > size.ws_row)
        *anchor -= (line - size.ws_row) * size.ws_col;
}
