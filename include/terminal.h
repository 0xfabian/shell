#pragma once

#include <iostream>
#include <termios.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>

void tty_raw();
void tty_restore();

void cursor_move_left(int steps = 1);