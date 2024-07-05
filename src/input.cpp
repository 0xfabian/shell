#include <input.h>
#include <shell.h>

using namespace std;

bool Input::get()
{
    tty_raw();

    data.clear();
    backup.clear();
    suggestion.clear();

    cursor = 0;
    selection = false;
    hist_index = -1;

    last_render_size = 0;
    last_cursor = 0;

    int res;
    while ((res = process_key()) > 0);

    tty_restore();

    return res < 0 ? false : true;
}

int Input::process_key()
{
    int key = get_key();

    if (isprint(key))
        insert(key);
    else
    {
        switch (key)
        {
        case ENTER:                     enter();                    return 0;
        case TAB:                       autocomplete();             break;
        case BACKSPACE:                 backspace();                break;
        case DELETE:                    delete_();                  break;

        case HOME:                      move_home(false);           break;
        case SHIFT_HOME:                move_home(true);            break;
        case END:                       move_end(false);            break;
        case SHIFT_END:                 move_end(true);             break;

        case ARROW_UP:                  history_up();               break;
        case ARROW_DOWN:                history_down();             break;

        case ARROW_RIGHT:               move_right(false, false);   break;
        case CTRL_ARROW_RIGHT:          move_right(true, false);    break;
        case SHIFT_ARROW_RIGHT:         move_right(false, true);    break;
        case SHIFT_CTRL_ARROW_RIGHT:    move_right(true, true);     break;

        case ARROW_LEFT:                move_left(false, false);    break;
        case CTRL_ARROW_LEFT:           move_left(true, false);     break;
        case SHIFT_ARROW_LEFT:          move_left(false, true);     break;
        case SHIFT_CTRL_ARROW_LEFT:     move_left(true, true);      break;

        case CTRL_KEY('a'):             select_all();               break;
        case CTRL_KEY('d'):                                         return -1;
        }
    }

    if (selection && cursor == selection_anchor)
        selection = false;

    find_suggestion();
    render();

    return 1;
}

void Input::insert(int ch)
{
    hist_index = -1;

    if (selection)
        delete_selection();

    data.insert(data.begin() + cursor, ch);
    cursor++;
}

void Input::backspace()
{
    hist_index = -1;

    if (selection)
        delete_selection();
    else if (cursor > 0)
    {
        cursor--;
        data.erase(data.begin() + cursor);
    }
}

void Input::delete_()
{
    hist_index = -1;

    if (selection)
        delete_selection();
    else if (cursor != data.size())
        data.erase(data.begin() + cursor);
}

void Input::delete_selection()
{
    size_t sel_start = cursor < selection_anchor ? cursor : selection_anchor;
    size_t sel_end = cursor > selection_anchor ? cursor : selection_anchor;

    data.erase(data.begin() + sel_start, data.begin() + sel_end);

    selection = false;

    if (cursor > selection_anchor)
        cursor = selection_anchor;
}

void Input::test_selection(bool shift)
{
    if (shift && !selection)
    {
        selection = true;
        selection_anchor = cursor;
    }
}

void Input::enter()
{
    sh->add_history(data);

    cout << endl;
}

bool starts_with(const string& str, const string& prefix)
{
    if (prefix.length() > str.length())
        return false;

    return str.compare(0, prefix.length(), prefix) == 0;
}

void Input::find_suggestion()
{
    if (!data.empty())
    {
        for (int i = sh->history.size() - 1; i >= 0; i--)
        {
            if (starts_with(sh->history[i], data))
            {
                suggestion = sh->history[i];
                return;
            }
        }
    }

    suggestion = "";
}

void Input::autocomplete()
{
    if (suggestion.empty())
        return;

    hist_index = -1;

    data = suggestion;
    selection = false;
    cursor = data.size();
}

void Input::move_home(bool shift)
{
    test_selection(shift);

    if (!shift && selection)
        selection = false;

    cursor = 0;
}

void Input::move_end(bool shift)
{
    test_selection(shift);

    if (!shift && selection)
        selection = false;

    cursor = data.size();
}

int Input::prev_offset()
{
    int off;

    for (off = 1; cursor - off > 0; off++)
        if (isspace(data[cursor - off - 1]) && !isspace(data[cursor - off]))
            break;

    return off;
}

int Input::next_offset()
{
    int off;

    for (off = 1; cursor + off < data.size(); off++)
        if (isspace(data[cursor + off - 1]) && !isspace(data[cursor + off]))
            break;

    return off;
}

void Input::move_right(bool ctrl, bool shift)
{
    test_selection(shift);

    if (!shift && selection)
    {
        selection = false;
        cursor = cursor > selection_anchor ? cursor : selection_anchor;
    }
    else
    {
        int offset = ctrl ? next_offset() : 1;

        if (cursor < data.size())
            cursor += offset;
    }
}

void Input::move_left(bool ctrl, bool shift)
{
    test_selection(shift);

    if (!shift && selection)
    {
        selection = false;
        cursor = cursor < selection_anchor ? cursor : selection_anchor;
    }
    else
    {
        int offset = ctrl ? prev_offset() : 1;

        if (cursor > 0)
            cursor -= offset;
    }
}

void Input::history_up()
{
    if (hist_index == sh->history.size() - 1)
        return;

    if (hist_index == -1)
        backup = data;

    if (hist_index < static_cast<int>(sh->history.size()) - 1)
    {
        hist_index++;
        data = sh->get_history(hist_index);

        selection = false;
        cursor = data.size();
    }
}

void Input::history_down()
{
    if (hist_index == -1)
        return;

    hist_index--;

    if (hist_index == -1)
        data = backup;
    else
        data = sh->get_history(hist_index);

    selection = false;
    cursor = data.size();
}

void Input::select_all()
{
    selection = true;
    selection_anchor = 0;
    cursor = data.size();
}

void Input::render()
{
    size_t sel_start = cursor < selection_anchor ? cursor : selection_anchor;
    size_t sel_end = cursor > selection_anchor ? cursor : selection_anchor;

    cursor_move_left(last_cursor);

    cout << "\e[0m" << string(last_render_size, ' ');

    cursor_move_left(last_render_size);

    int render_size = suggestion.size() > data.size() ? suggestion.size() : data.size();

    string output;

    if (selection)
        output = data.substr(0, sel_start) + "\e[44;96m" + data.substr(sel_start, sel_end - sel_start) + "\e[0m" + data.substr(sel_end);
    else
        output = data;

    if (suggestion.size() > data.size())
        output += "\e[0m\e[38;5;242m" + suggestion.substr(data.size());

    cout << output;

    cursor_move_left(render_size - cursor);

    last_render_size = render_size;
    last_cursor = cursor;
}

int Input::get_key()
{
    int c = getchar();

    if (c == EOF)
    {
        tty_restore();
        exit(2);
    }

    if (c == '\e')
    {
        char seq[5];

        if ((seq[0] = getchar()) == -1)     return '\e';
        if (seq[0] != '[')                  return seq[0];
        if ((seq[1] = getchar()) == -1)     return '\e';

        if (isdigit(seq[1]))
        {
            if ((seq[2] = getchar()) == -1)     return '\e';
            if (seq[2] == '~' && seq[1] == '3') return DELETE;

            if (seq[1] == '1')
            {
                if ((seq[3] = getchar()) == -1)  return '\e';
                if ((seq[4] = getchar()) == -1)  return '\e';

                switch (seq[2])
                {
                case ';':
                    if (seq[3] == '5')
                    {
                        if (seq[4] == 'C') return CTRL_ARROW_RIGHT;
                        if (seq[4] == 'D') return CTRL_ARROW_LEFT;
                    }

                    if (seq[3] == '6')
                    {
                        if (seq[4] == 'C')  return SHIFT_CTRL_ARROW_RIGHT;
                        if (seq[4] == 'D') return SHIFT_CTRL_ARROW_LEFT;
                    }

                    if (seq[3] == '2')
                    {
                        if (seq[4] == 'C') return SHIFT_ARROW_RIGHT;
                        if (seq[4] == 'D') return SHIFT_ARROW_LEFT;
                        if (seq[4] == 'H') return SHIFT_HOME;
                        if (seq[4] == 'F') return SHIFT_END;
                    }
                }
            }
        }
        else
        {
            switch (seq[1])
            {
            case 'A': return ARROW_UP;
            case 'B': return ARROW_DOWN;
            case 'C': return ARROW_RIGHT;
            case 'D': return ARROW_LEFT;
            case 'H': return HOME;
            case 'F': return END;
            }
        }

        return seq[1];
    }
    else
        return c;
}