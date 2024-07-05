#pragma once

#include <iostream>
#include <termios.h>

void tty_raw();
void tty_restore();

void cursor_move_left(int steps = 1);
void cursor_move_right(int steps = 1);