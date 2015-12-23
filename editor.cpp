#include <iostream>
#include <fstream>
#include <stdexcept>
#include <curses.h>
#include <signal.h>

using namespace std;
#include "sequence.h"
#include "track.h"
#include "controller.h"

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

int mixer_view(int);
int sequencer_view(int);
int help_view(int);


bool load_sequencer();
bool load_instruments();
bool save_sequencer();
bool save_mixer();

string read_prompt(const string &prompt);
int read_int(const string &prompt);
bool read_yn(const string &prompt);

void print_error(const string &error);
void print_status(const string &status);
void clear_screen();

struct sequencer_state {
    string filename;
    bool has_name {false};
    bool need_save {false};
    int offset {0};
    int idx {0};
    int t_idx {0};
};

struct mixer_state {
    string filename;
    bool has_name {false};
    bool need_save {false};
    int idx {0};
};

bool quit_now = false;

int width;
int height;
WINDOW *win;

sequencer_state ss;
mixer_state ms;

int mixer_view(int previous_view) {
    int c;
    while (1) {

        clear_screen();
        
        mvprintw(0, 0, "INSTRUMENTS %s%s",
                ms.has_name ? ms.filename.c_str() : "[new file]",
                ms.need_save ? "*" : " ");

        mvprintw(1, 0, "1-4: Navigate, S: save, L: load, E: edit track, D: delete track, V: volume");

        float buffer[width];
        for (int i = 0; i < 4; ++i) {
            int y = 2 + ((height - 3) * i) / 4;
            mvprintw(y, 2, "TRACK %d:", i + 1);
            if (tracks[i].enabled) {
                mvprintw(y++, 10, "| volume=[%3d] | value=[%s]", tracks[i].volume, tracks[i].instrument.c_str());
            } else {
                mvprintw(y++, 10, "| volume=[N/A]  | value={ø}");
            }

            ++y;
            int slice_height = (height - 3) / 4 - 3;
            if (tracks[i].enabled) {
                eval_with_params(i, 60, 1.0 / 220, width, buffer);
            }

            if (i == ms.idx) {
                for (int j = 0; j < slice_height; ++j) {
                    mvaddch(y + j, 0, ':');
                }
            }

            int last = y + slice_height * (1 + (double)tracks[i].volume / -100 * buffer[0]) / 2;
            if (last < y) last = y;
            if (last > y + slice_height) last = y + slice_height;
            for (int j = 1; j < width; ++j) {
                mvaddch(y + slice_height / 2, j, '-');
                if (tracks[i].enabled) {
                    int grid_y = y + slice_height * (1 + (double)tracks[i].volume / -100 * buffer[j]) / 2;
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
        switch (c) {
            case '1': ms.idx = 0; break;
            case '2': ms.idx = 1; break;
            case '3': ms.idx = 2; break;
            case '4': ms.idx = 3; break;
            case 's':
            case 'S':
                save_mixer();
                break;
            case 'e':
            case 'E':
            case '\n':
                // edit
                {
                    string ex = read_prompt("new value");
                    if (ex != "") {
                        string prev = tracks[ms.idx].instrument;
                        try {
                            tracks[ms.idx].instrument = ex;
                            setup_instrument(ms.idx, tracks[ms.idx].instrument);
                            ms.need_save = true;
                            tracks[ms.idx].enabled = true;
                            print_status("");
                        } catch (exception &e) {
                            // unable to compile
                            print_error(e.what());
                            tracks[ms.idx].instrument = prev;
                        }
                    } else {
                        print_status("aborted");
                    }
                }
                break;
            case 'd':
            case 'D':
                if (read_yn("delete instrument?")) {
                    if (tracks[ms.idx].enabled) {
                        remove_instrument(ms.idx);
                        ms.need_save = true;
                    }
                }
                break;
            case 'v':
            case 'V':
                // volume
                {
                    int volume = read_int("volume [0-100]");
                    if (volume < 0) volume = 0;
                    else if (volume > 100) volume = 100;
                    tracks[ms.idx].volume = volume;
                    ms.need_save = true;
                } 
                break;
            case 'k':
            case 'K':
            case KEY_UP:
                // up 
                if (ms.idx > 0) --ms.idx;
                break;
            case 'j':
            case 'J':
            case KEY_DOWN:
                // down
                if (ms.idx < 3) ++ms.idx;
                break;
            case 'l':
            case 'L':
                if (ms.need_save && read_yn("save tracks?")) {
                    if (!save_mixer())
                        break;
                }
                load_instruments();
                break;
            case 'q':
            case 'Q':
                if (read_yn("save and quit?")) {
                    save_and_quit();
                }
                break;
            case '\t':
                return previous_view;
            case 'h':
            case 'H':
                return HELP_VIEW;
        } 

        update();
    }

    return 0;
}


int sequencer_view(int previous_view) {
    
    while (1) {
        clear_screen();
        mvprintw(0, 0, "SEQUENCER %s%s",
                ss.has_name ? ss.filename.c_str() : "[new file]",
                ss.need_save ? "*" : " ");
        mvprintw(0, 30, "PATTERN: [");
        for (int i = 0; i < 4; ++i) {
            if (i == ss.idx) {
                attron(A_STANDOUT);
            }
            mvprintw(0, 41 + 3 * i, "%d", i + 1);
            if (i == ss.idx) {
                attroff(A_STANDOUT);
            }
        }
        mvprintw(0, 51, " ]");

        update();

        char c = getch();
        switch (c) {
            case '1': ss.idx = 0; break;
            case '2': ss.idx = 1; break;
            case '3': ss.idx = 2; break;
            case '4': ss.idx = 3; break;
            case 's':
            case 'S':
                save_sequencer();
                break;
            case '\t':
                return previous_view;
            case 'q':
            case 'Q':
                if (read_yn("save and quit?")) {
                    save_and_quit();
                }
                break;
            case 'h':
            case 'H':
                return HELP_VIEW;

        }
    }
    
    return 0;
}


int help_view(int previous_view) {
    
    mvprintw(0,  0, "HELP (Press [H] at any time to see this message)");
    mvprintw(2,  0, "[SPACE] Play / pause");
    mvprintw(3,  0, "[TAB]   Switch view");
    mvprintw(4,  0, "[Q]     Save and quit");

    mvprintw(6,  4, "MIXER");
    mvprintw(8,  4, "[S]   Save");
    mvprintw(9,  4, "[L]   Load");
    mvprintw(10, 4, "[E]   Edit instrument");
    mvprintw(11, 4, "[V]   Change volume");
    mvprintw(12, 4, "[J]   Next instrument");
    mvprintw(13, 4, "[K]   Last instrument");
    mvprintw(14, 4, "[1-4] Select instrument");
    mvprintw(15, 4, "[P]   Play note");

    mvprintw(6,  32, "SEQUENCER");
    mvprintw(8,  32, "[S]   Save song");
    mvprintw(9,  32, "[L]   Load song");
    mvprintw(10, 32, "[E]   Edit sequence");
    mvprintw(11, 32, "[1-4] Select sequence");
    mvprintw(12, 32, "[I]   Select instrument");
    mvprintw(13, 32, "[K]   In edit mode: Go up");
    mvprintw(14, 32, "[J]   In edit mode: Go down");
    mvprintw(15, 32, "[H]   In edit mode: Go left");
    mvprintw(16, 32, "[L]   In edit mode: Go right");
    mvprintw(17, 32, "[N]   In edit mode: Add note");
    mvprintw(16, 32, "[X]   In edit mode: Delete note");
    
    update();
    getch();
    clear_screen();

    if (previous_view == HELP_VIEW) 
        previous_view = MIXER_VIEW;
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
    scanw("%d", &val);
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
    for (int i = 0; i < height - 1; ++i) {
        move(i, 0);
        clrtoeol();
    }
}


bool save_mixer() {
    // save
    while (1) {
        string filename = ms.filename;
        if (!ms.has_name) {
            filename = read_prompt("instruments file");
            if (filename == "")
                return false;
        }

        ofstream out("/opt/chippy_files/" + filename);
        if (out.is_open()) {
            for (int i = 0; i < 4; ++i) {
                out << (i + 1) << " " << tracks[i] << endl;
            }
            out.close();
            ms.has_name = true;
            ms.need_save = false;
            ms.filename = filename;
            print_status("saved to /opt/chippy_files/" + filename);
            return true;
        } else {
            print_error("unable to open '" + filename + "'");
            ms.has_name = false;
        }
    }
}


bool load_instruments() {
    while (1) {
load_instruments_begin:
        string filename = read_prompt("load tracks");
        if (filename == "") {
            print_status("aborted");
            return false;
        }
        ifstream in("/opt/chippy_files/" + filename);
        if (in.is_open()) {
            for (int i = 0; i < 4; ++i) {
                remove_instrument(i);
            }

            ms.filename = "";
            ms.need_save = false;
            ms.has_name = false;

            mixer_state ms_new;
            track t_new[4];
            ms_new.filename = filename;
            ms_new.need_save = false;
            ms_new.has_name = true;
            int i;
            while (in >> i) {
                if (i < 1 || i > 4) {
                    print_error("malformed input file");
                    in.close();
                    goto load_instruments_begin;
                }
                in >> t_new[i-1];
                if (t_new[i-1].enabled) {
                    try {
                        setup_instrument(i - 1, t_new[i - 1].instrument);
                    } catch (exception &e) {
                        print_error("cannot compile instruments");
                    }
                }
            }
            ms = ms_new;
            for (int i = 0; i < 4; ++i)
                tracks[i] = t_new[i];
            print_status("loaded /opt/chippy_files/" + filename);
            return true;
        } else {
            print_error("unable to open " + filename);
        }
    }
}


bool save_sequencer() {

    return false;
}


bool load_sequencer() {
    
    return false;
}


void save_and_quit() {
    quit_now = true;
    if (ms.need_save) {
        if (read_yn("save mixer?")) {
            save_mixer();
        }
    }
    if (ss.need_save) {
        if (read_yn("save sequences?")) {
            save_sequencer();
        }
    }
    cleanup();
    exit(0);
}


void onsignal(int signum) {

    if (!quit_now) {
        save_and_quit();
    }

    cleanup();
    exit(0);
}


void update() {
    refresh();
    getmaxyx(win, height, width);
}


void cleanup() {
    delwin(win);
    endwin();
    refresh();
}


int main(void) {

    // init stuff
    if (!(win = initscr())) {
        cerr << "initscr() failed\n";
        return 1;
    }

    curs_set(0);
    noecho();
    getmaxyx(win, height, width);

    signal(SIGINT,  onsignal);
    signal(SIGHUP,  onsignal);
    signal(SIGTERM, onsignal);

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

    while (view) {
        view = view_func[view-1](view);
    }

    cleanup();
}

