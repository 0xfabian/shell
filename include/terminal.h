#pragma once

#include <iostream>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>

void tty_raw();
void tty_restore();

void set_cursor(size_t pos);
size_t get_cursor();

void ajust_for_scroll(size_t* anchor, size_t offset);