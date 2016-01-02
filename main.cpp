#include <iostream>
#include <fstream>
#include <stdexcept>
#include <curses.h>
#include <signal.h>

using namespace std;
#include "project.h"
#include "controller.h"
#include "util.h"
#include "ui.h"
#include "mixer_view.h"
#include "sequencer_view.h"
#include "dj_view.h"

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


int main(int argc, char **argv) {

    trace("starting");
    controller::init();
    if (argc > 1) {
        filename = argv[1];
        const string project_filename = "/opt/chippy_files/" + filename + ".chip";
        ifstream in(project_filename);
        if (!in.is_open()) {
            cerr << "unable to open " << project_filename << endl;
            return 1;
        }
        in >> controller::current_project;
        has_name = true;
        for (int i = 0; i < NUM_INSTRUMENTS; ++i) {
            if (controller::instruments[i].enabled) {
                controller::setup_instrument(i, controller::instruments[i].value);
            }
        }
    }

    ui_init();

    signal(SIGINT,  onsignal);
    signal(SIGHUP,  onsignal);
    signal(SIGTERM, onsignal);

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

