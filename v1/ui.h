#ifndef ui_h
#define ui_h

#include <string>

#define MIXER_VIEW      1
#define SEQUENCER_VIEW  2
#define DJ_VIEW         3
#define HELP_VIEW       4

void ui_init(void);

void onsignal(int);
void cleanup(void);
void update(void);
void save_and_quit(void);

void before_playing(void);
void done_playing(void);
bool play_until_input(void);

bool load_project();
bool save_project();

string read_prompt(const string &prompt);
int read_int(const string &prompt);
bool read_yn(const string &prompt);

void print_error(const string &error);
void print_status(const string &status);
void clear_status();
void clear_screen();

extern string filename;
extern bool has_name;
extern bool need_save;
extern bool edit_mode;
extern int offset_time;
extern int offset_note;
extern int s_idx;
extern int i_idx;
extern int cursor_x;
extern int cursor_y;
extern uint16_t nl_last[];
extern const char note_names[13];
extern bool quit_now;
extern bool is_playing;

extern const char note_chars[][4];

extern int width;
extern int height;


#endif
