#include <curses.h>
#include <string>

using namespace std;
#include "mixer_view.h"
#include "util.h"
#include "project.h"
#include "controller.h"
#include "ui.h"

int mixer_view(int previous_view) {
    int c;
    while (1) {

        clear_screen();
        
        mvprintw(0, 0, "%s%s MIXER",
                has_name ? filename.c_str() : "[new file]",
                need_save ? "*" : " ");

        mvprintw(1, 0, "project volume: [%d]", controller::current_project.volume);

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
                        controller::remove_instrument(i_idx);
                        need_save = true;
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
                    controller::instruments[i_idx].volume = volume;
                    need_save = true;
                } 
                break;
            case 'k':
            case 'K':
            case KEY_UP:
                // up 
                if (i_idx > 0) --i_idx;
                break;
            case 'j':
            case 'J':
            case KEY_DOWN:
                // down
                if (i_idx < 7) ++i_idx;
                break;
            case 'l':
            case 'L':
                if (need_save && read_yn("save project?")) {
                    if (!save_project())
                        break;
                }
                load_project();
                break;
            case 'q':
            case 'Q':
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
                before_playing();
                controller::play_sequence(s_idx, play_until_input);
                done_playing();
                break;
            case 'p':
            case 'P':
                // play note
                if (controller::instruments[i_idx].enabled) {
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

