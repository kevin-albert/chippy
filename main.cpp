#include <iostream>
#include <fstream>
#include <stdexcept>
#include <curses.h>
#include <signal.h>

using namespace std;
#include "project.h"
#include "controller.h"
#include "util.h"

#define MIXER_VIEW      1
#define SEQUENCER_VIEW  2
#define HELP_VIEW       3

string logo[] = {
"   _____ _    _ _____ _____  _______     __",
"  / ____| |  | |_   _|  __ \\|  __ \\ \\   / /",
" | |    | |__| | | | | |__) | |__) \\ \\_/ / ",
" | |    |  __  | | | |  ___/|  ___/ \\   /  ",
" | |____| |  | |_| |_| |    | |      | |   ",
"  \\_____|_|  |_|_____|_|    |_|      |_|   ",
"",
" [S] Sequencer",
" [M] Mixer",
" [H] Help"
};

void onsignal(int);
void cleanup(void);
void save_and_quit(void);
void update(void);

void before_playing(void);
void done_playing(void);
bool play_until_input(void);

int mixer_view(int);
int sequencer_view(int);
int help_view(int);
int dj_view(int);

inline int max(int a, int b) {
    return a > b ? a : b;
}

inline int min(int a, int b) {
    return a < b ? a : b; 
}

bool load_project();
bool save_project();

string read_prompt(const string &prompt);
int read_int(const string &prompt);
bool read_yn(const string &prompt);

void print_error(const string &error);
void print_status(const string &status);
void clear_status();
void clear_screen();

string filename;
bool has_name   {false};
bool need_save  {false};
bool edit_mode  {false};
int offset_time {0};
int offset_note {48};
int s_idx       {0};
int i_idx       {0};
int cursor_x    {0};
int cursor_y    {12};
uint16_t nl_last[] = {4, 4, 4, 4, 4, 4, 4, 4,
                      4, 4, 4, 4, 4, 4, 4, 4};
const char note_names[13] = "A BC D EF G ";
bool quit_now = false;
bool is_playing = false;

const char note_chars[][4] = {
    "[=]",
    "{+}",
    "(*)",
    "<#>",
    "\"'\"",
    "|%|",
    "/$/",
    "\\@\\"
};

int width;
int height;
WINDOW *win;

int mixer_view(int previous_view) {
    int c;
    while (1) {

        clear_screen();
        
        mvprintw(0, 0, "%s%s MIXER",
                has_name ? filename.c_str() : "[new file]",
                need_save ? "*" : " ");

        mvprintw(1, 0, "1-8: Navigate, S: save, L: load, E: edit track, D: delete track, V: volume");

        float buffer[width];
        int i_start = i_idx / 4;
        int i_end = i_start + 4;
        int height_offset = 0;
        for (int i = i_start; i < i_end; ++i) {
            int y = 2 + ((height - 3) * (i - i_start)) / 4;
            mvprintw(y, 2, "INSTRUMENT %d:", i + 1);
            instrument &inst = controller::instruments[i];
            if (inst.enabled) {
                mvprintw(y++, 15, " volume=[%3d] | value=[ %s ]", inst.volume, inst.value.c_str());
            } else {
                mvprintw(y++, 15, " volume=[N/A]  | value=[ ø ]");
            }

            ++y;
            int slice_height = (height - 3) / 4 - 3;
            if (inst.enabled) {
                controller::eval_with_params(i, 60, 1.0 / 220, width, buffer);
            }

            if (i == i_idx) {
                for (int j = 0; j < slice_height; ++j) {
                    mvaddch(y + j, 0, '*');
                }
            }

            int last = y + slice_height * (1 - (double) inst.volume / 100 * buffer[0]) / 2;
            if (last < y) last = y;
            if (last > y + slice_height) last = y + slice_height;
            for (int j = 1; j < width; ++j) {
                // print center line
                mvaddch(y + slice_height / 2, j, '-');
                if (inst.enabled) {
                    int grid_y = y + slice_height * (1 - (double) inst.volume / 100 * buffer[j]) / 2;
                    if (grid_y < y) grid_y = y;
                    if (grid_y > y + slice_height) grid_y = y + slice_height;

                    char sym = '=';
                    if (grid_y > last) {
                        sym = '\\';
                        if (j > 1)
                            mvaddch(last, j - 1, sym);
                        while (grid_y > ++last)
                           mvaddch(last, j, '|'); 
                    }
                    else if (grid_y < last) {
                        sym = '/';
                        if (j > 1)
                            mvaddch(last, j - 1, sym);
                        while (grid_y < --last)
                           mvaddch(last, j, '|'); 

                    }
                    mvaddch(grid_y, j, sym);
                }
            }
        }

        update();
        c = getch();
        clear_status();
mixer_view_switch:
        switch (c) {
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
                i_idx = c - 49;
                break;
            case 's':
            case 'S':
                save_project();
                break;
            case 'e':
            case 'E':
            case '\n':
            case ':':
            case ';':
                // edit
                {
                    string ex = read_prompt("new value");
                    if (ex != "") {
                        string prev = controller::instruments[i_idx].value;
                        try {
                            controller::instruments[i_idx].value = ex;
                            controller::setup_instrument(i_idx, controller::instruments[i_idx].value);
                            need_save = true;
                            controller::instruments[i_idx].enabled = true;
                            clear_status();
                        } catch (exception &e) {
                            // unable to compile
                            print_error(e.what());
                            controller::instruments[i_idx].value = prev;
                        }
                    } else {
                        print_status("cancelled");
                    }
                }
                break;
            case 'd':
            case 'D':
                if (read_yn("delete instrument?")) {
                    if (controller::instruments[i_idx].enabled) {
                        trace("deleting instrument " << i_idx);
                        controller::remove_instrument(i_idx);
                        need_save = true;
                    }
                }
                break;
            case 'v':
            case 'V':
                // volume
                {
                    trace("changing volume");
                    int volume = read_int("volume [0-100]");
                    if (volume < 0) volume = 0;
                    else if (volume > 100) volume = 100;
                    controller::instruments[i_idx].volume = volume;
                    need_save = true;
                } 
                break;
            case 'k':
            case 'K':
            case KEY_UP:
                // up 
                if (i_idx > 0) --i_idx;
                trace("changing to instrument " << i_idx);
                break;
            case 'j':
            case 'J':
            case KEY_DOWN:
                // down
                if (i_idx < 7) ++i_idx;
                trace("changing to instrument " << i_idx);
                break;
            case 'l':
            case 'L':
                trace("loading new project");
                if (need_save && read_yn("save project?")) {
                    if (!save_project())
                        break;
                }
                load_project();
                break;
            case 'q':
            case 'Q':
                trace("quitting");
                if (read_yn("save and quit?")) {
                    save_and_quit();
                }
                break;
            case '\t':
                return SEQUENCER_VIEW;
            case 'h':
            case 'H':
                return HELP_VIEW;
            case ' ':
                // play sequence
                trace("mixer view: play sequence");
                before_playing();
                controller::play_sequence(s_idx, play_until_input);
                done_playing();
                break;
            case 'p':
            case 'P':
                // play note
                if (controller::instruments[i_idx].enabled) {
                    trace("mixer view: preview instrument");
                    before_playing();
                    controller::play_note(i_idx, 60, play_until_input);
                    done_playing();
                }
                break;

        } 

        update();
    }

    return 0;
}


int sequencer_view(int previous_view) {
   
    if (edit_mode) {
        curs_set(1);
    }

    evt_note new_note;
    new_note.length = 8;
    new_note.start_vel = new_note.end_vel = 100;

    bool adding_note = false;

    while (1) {
        clear_screen();
        mvprintw(0, 0, "%s%s SEQUENCER",
                has_name ? filename.c_str() : "[new file]",
                need_save ? "*" : " ");
        mvprintw(0, 30, "PATTERN: [");
        for (int i = 0; i < NUM_SEQUENCES; ++i) {
            if (i == s_idx) {
                attron(A_STANDOUT);
            }
            mvprintw(0, 41 + 4 * i, "%2d", i + 1);
            if (i == s_idx) {
                attroff(A_STANDOUT);
            }
        }
        mvaddch(0, 42 + 4 * NUM_SEQUENCES, ']');

        sequence &seq = controller::sequences[s_idx];
        mvprintw(0, 46 + 4 * NUM_SEQUENCES, "bpm=[%d] | ts=[%d/4] | %d measures", controller::current_project.bpm, seq.ts, seq.length);
        mvprintw(1, 27, "INSTRUMENT: [");
        for (int i = 0; i < 8; ++i) {
            if (i == i_idx) {
                attron(A_STANDOUT);
            }
            mvprintw(1, 41 + 3 * i, "%d", i + 1);
            if (i == i_idx) {
                attroff(A_STANDOUT);
            }
        }

        mvaddch(1, 64, ']');
        // top bar
        for (int i = offset_time ? 0 : 3; i < width; ++i) {
            mvaddch(2, i, '-');
        }


        // render the current sequence
        int top = 2;
        int bottom = height - 2;
        int left = 3;
        int right = width;
        int screen_height = bottom - top;
        int screen_width = right - left;

        int measure_length = 16 * seq.ts;

        for (int i = left - offset_time % measure_length; i < width; i += measure_length) {
            if (i >= left) {
                mvprintw(2, i, "%d", (i + offset_time) / measure_length + 1);
            }
        }
        
        // note lines
        for (int i = screen_height - 1; i >= 0; --i) {
            int note = offset_note + i;
            char name = note_names[note % 12];
            int y = top + screen_height - i;
            if (edit_mode && i == cursor_y) {
                attron(A_STANDOUT);
            }
            if (name == ' ') {
                mvprintw(y, 0, "   ", name);
                if (edit_mode && i == cursor_y) {
                    attroff(A_STANDOUT);
                }
            } else {
                mvprintw(y, 0, "%c%d", name, note / 12);
                if (edit_mode && i == cursor_y) {
                    attroff(A_STANDOUT);
                }

                if (name == 'A') {
                    for (int j = left; j < right; ++j) {
                        mvaddch(y, j, '.');
                    }
                }
            }
        }

        // bar lines
        int k = 0;
        for (int i = left - offset_time % measure_length; i < right; i += 16) {
            if (i >= left) {
                if (k % seq.ts) {
                    for (int j = top + 1; j <= bottom; ++j) {
                        mvaddch(j, i, '.');
                    }
                } else {
                    for (int j = top + 1; j <= bottom; ++j) {
                        mvaddch(j, i, '|');
                    }
                }
            }
            ++k;
        }

        int selected_idx = -1;
        int note_idx = 0;

        if (edit_mode) {
            if (adding_note) {
                int y = top + screen_height - new_note.start_note + offset_note;
                int x = max(new_note.start - offset_time, 0);
                attron(A_STANDOUT);
                mvaddch(y, left + x, note_chars[i_idx][0]);
                attroff(A_STANDOUT);
            }

            // display notes
            for (evt_note &n: seq.notes) {
                if ((   (n.start_note >= offset_note && n.end_note < offset_note + screen_height) ||
                        (n.start_note >= offset_note && n.end_note < offset_note + screen_height)
                    ) &&
                    (n.start + n.length > offset_time || n.start < offset_time + width)) {

                    char disp = note_chars[n.instrument][1];
                    int start = n.start - offset_time;
                    int end = n.start - offset_time + n.length;
                    int ystart = top + screen_height - n.start_note + offset_note;
                    int ydiff = top + screen_height - n.end_note + offset_note - ystart;
                    int xdiff = end - start;
                    int xval = 1;
                    int note_top = ydiff > 0 ? ystart + ydiff : ystart;
                    int note_bottom = ydiff > 0 ? ystart : ystart + ydiff;

                    // draw this note
                    if (n.instrument == i_idx) {
                        // see if we've highlighted a note
                        if (    !adding_note && 
                                selected_idx    == -1 && 
                                start           <= cursor_x && 
                                end             >  cursor_x &&
                                note_top        >= bottom - cursor_y &&
                                note_bottom     <= bottom - cursor_y) {

                            mvprintw(height - 1, 0, "[%d] note %d -> %d, vel: %d -> %d", 
                                     n.start, n.start_note, n.end_note, n.start_vel, n.end_vel);

                            selected_idx = note_idx;
                            // this note is for the currently selected instrument
                            attron(A_STANDOUT);
                        }

                    }

                    // display the beginning of the note as bold
                    if (start >= 0 && end - start == n.length) {
                        attron(A_BOLD);
                        mvaddch(ystart, left + start++, note_chars[n.instrument][0]);
                        attroff(A_BOLD);
                    }

                    while (start < end) {
                        ++xval;
                        if (start >= 0 && start < screen_width) {
                            if (start == end - 1) {
                                disp = note_chars[n.instrument][2];
                                attron(A_BOLD);
                            }
                            int y = ystart + ydiff * xval / xdiff;
                            if (y > top && y <= bottom) 
                                mvaddch(y, left + start, disp);
                        }
                        ++start;
                    }

                    attroff(A_BOLD);
                    if (selected_idx == note_idx) {
                        // this note is for the currently selected instrument
                        attroff(A_STANDOUT);
                    }

                }
                
                ++note_idx;
            }

            // move the cursor
            move(bottom - cursor_y, cursor_x + left);
        }

        update();

        int c = getch();
        while (measure_length >= max(screen_width, 2)) {
            measure_length /= 2;
        }
        switch (c) {
            case '0':
                trace("change sequence: 0%");
                print_status("change sequence [1-9]");
                c = getch();
                if (c == '1') {
                    s_idx = 0;
                    break;
                } else {
                    goto select_sequence_gt2;
                }
            case '1':
                trace("change sequence: 1%");
                print_status("change sequence [10-16] (SPACE selects sequence 1)");
                c = getch();
                switch (c) {
                    case ' ':
                        s_idx = 0;
                        break;
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                        s_idx = 10 + c - 49;
                        break;
                }
                break;
select_sequence_gt2:
            case '2':
            case '3':
            case '4': 
            case '5': 
            case '6': 
            case '7': 
            case '8': 
            case '9': 
                trace("change sequence: 1-9");
                s_idx = c - 49;
                break;
            case 's':
            case 'S':
                save_project();
                break;
            case '\t':
                curs_set(0);
                return MIXER_VIEW;
            case 'q':
            case 'Q':
                if (read_yn("save and quit?")) {
                    save_and_quit();
                }
                break;
            case '[':
            case KEY_LEFT:
                if (edit_mode) {
                    // left + 1/16
                    if (cursor_x == 0) {
                        if (offset_time >= measure_length) {
                            offset_time -= measure_length;
                            cursor_x += measure_length - 1;
                        }
                    } else {
                        --cursor_x;
                    }
                }
                break;
            case 'h':
                if (edit_mode) {
                    // left + 1/4
                    if (cursor_x > 3) {
                        cursor_x -= 4;
                    } else if (offset_time > 0) {
                        offset_time -= measure_length;
                        cursor_x = measure_length - 4;
                    } else {
                        cursor_x = 0;
                    }
                } else {
                    curs_set(0);
                    return HELP_VIEW;
                }
                break;
            case 'H':
                if (edit_mode) {
                    // left + 1
                    if (cursor_x > measure_length - 1) {
                        cursor_x -= measure_length;
                    } else if (offset_time > 0) {
                        offset_time -= measure_length;
                    } else {
                        cursor_x = 0;
                    }
                } else {
                    curs_set(0);
                    return HELP_VIEW;
                }
                break;
            case ']':
            case KEY_RIGHT:
                if (edit_mode) {
                    // right + 1/16
                    if (cursor_x == right - 1) {
                        offset_time += measure_length;
                        cursor_x -= measure_length - 1;
                    } else {
                        ++cursor_x;
                    }
                }
                break;
            case 'l':
                if (edit_mode) {
                    // right + 1/14t
                    if (cursor_x < width - 4) {
                        cursor_x += 4;
                    } else {
                        offset_time += measure_length;
                        cursor_x -= measure_length - 4;
                    }
                } else {
                    load_project();
                }
                break;
            case 'L':
                if (edit_mode) {
                    // move right
                    if (cursor_x < width - measure_length) {
                        cursor_x += measure_length;
                    } else {
                        offset_time += measure_length;
                    }
                } else {
                    if (need_save) {
                        if (!save_project()) {
                            break;
                        }
                    }
                    load_project();
                }
                break;
            case 'j':
            case KEY_DOWN:
                if (edit_mode) {
                    if (cursor_y > 0) {
                        --cursor_y;
                    } else if (offset_note > 0) {
                        offset_note -= 6;
                        cursor_y += 5;
                    }
                }
                break;
            case 'J':
                if (edit_mode && offset_note > 0) {
                    if (cursor_y > 5) {
                        cursor_y -= 6;
                    } else if (offset_note > 0) {
                        offset_note -= 6;
                    }
                }
                break;
            case 'k':
            case KEY_UP:
                if (edit_mode) {
                    if (cursor_y < screen_height - 1) {
                        ++cursor_y;
                    } else if (offset_note + screen_height <= 246) {
                        offset_note += 6;
                        cursor_y -= 5;
                    }
                }
                break;
            case 'K':
                if (edit_mode && offset_note + screen_height <= 246) {
                    if (cursor_y < screen_height - 6) {
                        cursor_y += 6;
                    } else if (offset_note + screen_height <= 246) {
                        offset_note += 6;
                    }
                }
                break;
            case 'i':
            case 'I':
                print_status("change instrument [1-8]");
                c = getch();
                switch (c) {
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                        i_idx = c - 49;
                        clear_status();
                        break;
                    default: 
                        print_status("cancelled");
                        break;
                }
                break;
            case 'e': 
            case 'E': 
                edit_mode = !edit_mode;
                curs_set(edit_mode);
                break;
            case 'd':
            case 'D':
            case 'x':
            case 'X':
                if (edit_mode && selected_idx != -1) {
                    seq.notes[selected_idx] = seq.notes.back();
                    seq.notes.pop_back();
                    seq.update_natural_length();
                    clear_status();
                    need_save = true;
                }
                break;
            case 'n':
            case 'N':
                if (edit_mode) {
                    if (adding_note) {
                        new_note.length = cursor_x + offset_time - new_note.start;
                        new_note.end_note = offset_note + cursor_y;
                        nl_last[i_idx] = new_note.length;
                        seq.add_note(new_note);
                        need_save = true;
                    } else {
                        new_note.instrument = i_idx;
                        new_note.start_note = offset_note + cursor_y;
                        new_note.start = offset_time + cursor_x;
                        cursor_x += nl_last[i_idx];
                        if (cursor_x >= width) {
                            cursor_x = width-1;
                        }
                    }
                    adding_note = !adding_note;
                }
                break;
            case 't':
                // change time signature
                {
                    int ts = read_int("quarter notes per measure");
                    if (ts > 0) {
                        seq.ts = ts;
                        need_save = true;
                    }
                }
                break;
            case 'T':
                // change sequence length
                {
                    int length = read_int("sequence length in (number of measures)");
                    if (length > seq.natural_length) {
                        seq.length = length;
                        need_save = true;
                    }
                }
                break;
            case 'b':
            case 'B':
                {
                    int bpm = read_int("bpm");
                    if (bpm > 0) {
                        controller::current_project.bpm = bpm;
                        need_save = true;
                    }
                }
                break;
            case ' ':
                // play sequence
                trace("sequence view: play sequence");
                before_playing();
                controller::play_sequence(s_idx, play_until_input);
                done_playing();
                break;
            case 'p':
            case 'P':
                // play note
                trace("sequence view: preview note");
                before_playing();
                controller::play_note(i_idx, offset_note + cursor_y, play_until_input);
                done_playing();
                break;
        }
    }
    
    curs_set(0);
    return 0;
}


int help_view(int previous_view) {

    clear_screen();
    
    mvprintw(0,  0, "HELP (Press [H] at any time to see this message)");
    mvprintw(2,  0, "[SPACE]    Play / pause");
    mvprintw(3,  0, "[TAB]      Switch view");
    mvprintw(4,  0, "[Q]        Save and quit");

    mvprintw(6,  4, "MIXER");
    mvprintw(8,  4, "[S]        Save");
    mvprintw(9,  4, "[L]        Load");
    mvprintw(10, 4, "[E]        Edit instrument");
    mvprintw(11, 4, "[V]        Change volume");
    mvprintw(12, 4, "[J]        Next instrument");
    mvprintw(13, 4, "[K]        Last instrument");
    mvprintw(14, 4, "[1-8]      Select instrument");
    mvprintw(15, 4, "[SPACE]    Play note");

    mvprintw(6,  32, "SEQUENCER");
    mvprintw(8,  32, "[S]       Save song");
    mvprintw(9,  32, "[L]       Load song");
    mvprintw(10, 32, "[B]       Change BPM");
    mvprintw(11, 32, "[T]       Change sequence length");
    mvprintw(12, 32, "[SHIFT+T] Change time signature");
    mvprintw(13, 32, "[E]       Edit sequence");
    mvprintw(14, 32, "[1-16]    Select sequence");
    mvprintw(15, 32, "[I]       Select instrument");
    mvprintw(16, 32, "[K]       In edit mode: Go up");
    mvprintw(17, 32, "[J]       In edit mode: Go down");
    mvprintw(18, 32, "[H]       In edit mode: Go left");
    mvprintw(19, 32, "[L]       In edit mode: Go right");
    mvprintw(20, 32, "[N]       In edit mode: Add note");
    mvprintw(21, 32, "[X]       In edit mode: Delete note");
    
    update();
    char c = getch();
    clear_screen();
    
    if (c == 'q' || c == 'Q') {
        save_and_quit();
    }

    if (previous_view == HELP_VIEW) 
        previous_view = MIXER_VIEW;
    return previous_view;
}

int dj_view(int previous_view) {


    return previous_view;
}

void print_error(const string &error) {
    move(height - 1, 0);
    clrtoeol();
    mvprintw(height - 1, 0, "error: %s", error.c_str());
    update();
    getch();
}

void print_status(const string &status) {
    move(height - 1, 0);
    clrtoeol();
    mvprintw(height - 1, 0, "%s", status.c_str());
    update();
}

void clear_status() {
    move(height - 1, 0);
    clrtoeol();
    update();
}

string read_prompt(const string &prompt) {
    echo();
    curs_set(1);
    move(height - 1, 0);
    clrtoeol();
    mvprintw(height - 1, 0, "%s: ", prompt.c_str());
    update();
    char val[255];
    getstr(val);
    noecho();
    curs_set(0);
    update();
    return val;
}

int read_int(const string &prompt) {
    echo();
    curs_set(1);
    move(height - 1, 0);
    clrtoeol();
    mvprintw(height - 1, 0, "%s: ", prompt.c_str());
    update();
    int val;
    char fmt[3] = "%d";
    if (scanw(fmt, &val) == ERR) {
        val = 0;
    }
    noecho();
    curs_set(0);
    update();
    return val;
}

bool read_yn(const string &prompt) {
    move(height - 1, 0);
    clrtoeol();
    mvprintw(height - 1, 0, "%s [y/n]: ", prompt.c_str());
    move(height-1, 0);
    while (1) {
        char c = getch();
        switch (c) {
            case 'y':
            case 'Y':
                clrtoeol();
                return true;
            case 'n':
            case 'N':
                clrtoeol();
                return false;
        }
    }
}

void clear_screen() {
    clear();
}

bool save_project() {
    while (1) {
        string s_filename = filename;
        if (!has_name) {
            s_filename = read_prompt("project name");
            if (s_filename == "") {
                print_status("cancelled");
                return false;
            }
        }

        ofstream out("/opt/chippy_files/" + s_filename + ".chip");
        if (out.is_open()) {
            out << controller::current_project;
            out.close();
            has_name = true;
            need_save = false;
            filename = s_filename;
            print_status("saved to /opt/chippy_files/" + filename + ".chip");
            return true;
        } else {
            print_error("unable to open '" + s_filename + "'");
            has_name = false;
        }
    }
}

bool load_project() {
    while (1) {
load_project_begin:
        string l_filename = read_prompt("project name");
        if (l_filename == "") {
            print_status("cancelled");
            return false;
        }
        ifstream in("/opt/chippy_files/" + l_filename + ".chip");
        if (in.is_open()) {
            in >> controller::current_project;
            need_save = false;
            has_name = false;
            print_status("loaded /opt/chippy_files/" + l_filename + ".chip");
            filename = l_filename;
            need_save = false;
            has_name = true;
            for (int i = 0; i < NUM_INSTRUMENTS; ++i) {
                controller::remove_instrument(i);
                if (controller::instruments[i].enabled) {
                    controller::setup_instrument(i, controller::instruments[i].value);
                }
            }
            return true;
        } else {
            print_error("unable to open " + l_filename);
        }
    }
}

void save_and_quit() {
    quit_now = true;
    if (need_save) {
        if (read_yn("save project?")) {
            save_project();
        }
    }
    cleanup();
    exit(0);
}

void onsignal(int signum) {
    trace("signal = " << signum << ", quit_now = " << quit_now);
    if (!quit_now) {
        save_and_quit();
    }

    trace("invoking clenup()");
    cleanup();
    trace("exiting...");
    debug.close();
    exit(0);
}


void update() {
    refresh();
    getmaxyx(win, height, width);
}

void cleanup() {
    delwin(win);
    endwin();
    controller::destroy();
    refresh();
}

void before_playing() {
    mvprintw(0, width - 1, ">");
    nodelay(win, true);
}

bool play_until_input() {
    bool result = getch() == ERR;
    trace("haz input? " << result);
    return result;
}

void done_playing() {
    mvprintw(0, width - 1, " ");
    nodelay(win, false);
}

int main(void) {

    trace("started");
    // init stuff
    if (!(win = initscr())) {
        cerr << "initscr() failed\n";
        return 1;
    }

    curs_set(0);
    noecho();
    getmaxyx(win, height, width);

    trace("created window");

    signal(SIGINT,  onsignal);
    signal(SIGHUP,  onsignal);
    signal(SIGTERM, onsignal);

    controller::init();
    trace("set up controller");

    for (int i = 0; i < 10; ++i) {
        mvprintw(height / 2 - 5 + i, width / 2 - 22, logo[i].c_str());
    }

    int (*view_func[])(int) = {
        mixer_view,
        sequencer_view,
        help_view
    };

    int view = 0;
    while (!view) {
        char c = getch();
        switch (c) {
            case 's':
            case 'S':
                view = SEQUENCER_VIEW;
                break;
            case 'm':
            case 'M':
                view = MIXER_VIEW;
                break;
            case 'h':
            case 'H':
                view = HELP_VIEW;
                break;
            case 'q':
            case 'Q':
                cleanup();
                return 0;
        }
    }

    clear();

    int previous_view = HELP_VIEW;
    int next_view;
    while (view) {
        trace("switching from view " << previous_view << " to " << view);
        next_view = view_func[view-1](previous_view);
        trace("next view=" << next_view);
        previous_view = view;
        view = next_view;
    }

    cleanup();
    trace("exiting main function");
}

