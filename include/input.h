#pragma once

#include <string>
#include <dirent.h>
#include <terminal.h>

#define CTRL_KEY(key) ((key) & 0x1f)

enum Key
{
    TAB = '\t',
    ENTER = '\n',
    BACKSPACE = 127,
    HOME = 1000,
    END,
    DELETE,
    ARROW_LEFT,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    CTRL_ARROW_LEFT,
    CTRL_ARROW_RIGHT,
    SHIFT_ARROW_LEFT,
    SHIFT_ARROW_RIGHT,
    SHIFT_CTRL_ARROW_RIGHT,
    SHIFT_CTRL_ARROW_LEFT,
    SHIFT_HOME,
    SHIFT_END
};

struct Shell;

struct Input
{
    Shell* sh;
    std::string data;
    std::string backup;
    std::string suggestion;

    size_t input_anchor;
    size_t cursor;
    size_t selection_anchor;
    bool selection;
    int hist_index = -1;

    int last_render_size;
    size_t last_cursor;

    Input(Shell* _sh) : sh(_sh) {};

    bool get();

    int get_key();
    int process_key();

    void insert(int ch);
    void backspace();
    void delete_();
    void delete_selection();
    void test_selection(bool shift);

    void enter();

    void find_suggestion();
    void autocomplete();

    void move_home(bool shift);
    void move_end(bool shift);

    int prev_offset();
    int next_offset();

    void move_right(bool ctrl, bool shift);
    void move_left(bool ctrl, bool shift);

    void history_up();
    void history_down();

    void select_all();

    void render();
};