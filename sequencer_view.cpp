#include <curses.h>
#include <string>

using namespace std;
#include "sequencer_view.h"
#include "util.h"
#include "project.h"
#include "controller.h"
#include "ui.h"

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
        mvprintw(1, 46 + 4 * NUM_SEQUENCES, "project volume: [%d] | sequence volume: [%d]", controller::current_project.volume, controller::sequences[s_idx].volume);
        // top bar
        for (int i = offset_time ? 0 : 3; i < width; ++i) {
            mvaddch(3, i, '-');
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

        if (edit_mode) {
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
                print_status("change sequence [1-9]");
                c = getch();
                if (c == '1') {
                    s_idx = 0;
                    break;
                } else {
                    goto select_sequence_gt2;
                }
            case '1':
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
                before_playing();
                controller::play_sequence(s_idx, play_until_input);
                done_playing();
                break;
            case 'p':
            case 'P':
                // play note
                before_playing();
                controller::play_note(i_idx, offset_note + cursor_y, play_until_input);
                done_playing();
                break;
            case 'v':
                // volume
                {
                    int volume = read_int("sequence volume [0-100]");
                    if (volume < 0) volume = 0;
                    else if (volume > 100) volume = 100;
                    controller::sequences[s_idx].volume = volume;
                    need_save = true;
                } 
                break;
            case 'V':
                {
                    int volume = read_int("project volume [0-100]");
                    if (volume < 0) volume = 0;
                    else if (volume > 100) volume = 100;
                    controller::current_project.volume = volume;
                    need_save = true;
                }
        }
    }
    
    curs_set(0);
    return 0;
}

