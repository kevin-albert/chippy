#include <string>
#include <stdexcept>
#include <curses.h>

using namespace std;
#include "ui.h"
#include "util.h"
#include "controller.h"
#include "project.h"

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

void ui_init() {
    if (!(win = initscr())) {
        throw runtime_error("initscr() failed");
    }

    curs_set(0);
    noecho();
    getmaxyx(win, height, width);

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
                if (controller::instruments[i].enabled) {
                    controller::setup_instrument(i, controller::instruments[i].value);
                } else {
                    controller::remove_instrument(i);
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
    return result;
}

void done_playing() {
    mvprintw(0, width - 1, " ");
    nodelay(win, false);
}

