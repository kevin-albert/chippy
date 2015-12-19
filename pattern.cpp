#include <iomanip>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>

using namespace std;
#include "pattern.h"


template <typename T>
static T read_stream(istream &input) {
    T value = 0;
    input.read(reinterpret_cast<char*>(&value), sizeof value);
    cout << "read value: " << value << endl;
    return value;
}

template <typename T>
static void write_stream(ostream &output, const T value) {
    cout << "writing value: " << value << endl;
    output.write(reinterpret_cast<const char*>(&value), sizeof value);
}

istream &operator>>(istream &input, evt_note &note) {
    note = evt_note(
            read_stream<uint8_t> (input),   // inst
            read_stream<uint32_t>(input),   // start
            read_stream<uint16_t>(input),   // length
            read_stream<uint8_t> (input),   // note
            read_stream<uint8_t> (input));  // vel
    return input;
}

ostream &operator<<(ostream &output, const evt_note &note) {
    write_stream(output, (char) EVT_NOTE);
    write_stream(output, note.instrument);
    write_stream(output, note.start);
    write_stream(output, note.length);
    write_stream(output, note.note);
    write_stream(output, note.vel);
    return output;
}

istream &operator>>(istream &input, evt_slide &slide) {
    slide = evt_slide(
            read_stream<uint8_t> (input),   // inst
            read_stream<uint32_t>(input),   // start
            read_stream<uint16_t>(input),   // length
            read_stream<uint8_t> (input),   // start_note
            read_stream<uint8_t> (input),   // start_vel
            read_stream<uint8_t> (input),   // end_note
            read_stream<uint8_t> (input));  // end_vel
    return input;
}

ostream &operator<<(ostream &output, const evt_slide &slide) {
    write_stream(output, (char) EVT_SLIDE);
    write_stream(output, slide.instrument);
    write_stream(output, slide.start);
    write_stream(output, slide.length);
    write_stream(output, slide.start_note);
    write_stream(output, slide.start_vel);
    write_stream(output, slide.end_note);
    write_stream(output, slide.end_vel);
    return output;
}

istream &operator>>(istream &input, pattern &p) {
    p.notes.clear();
    p.slides.clear();

    char header[7];
    input.read(header, 6);
    header[6] = '\0';
    if (strcmp(header, "chippy")) {
        throw invalid_argument("invalid header");
    }

    char key;
    while (input.get(key) && key != KEY_DONE) {
        evt_note n;
        evt_slide s;
        cout << "got key: " << key << endl;
        switch (key) {
            case EVT_NOTE:
                input >> n;
                p.notes.push_back(n);
                break;
            case EVT_SLIDE:
                input >> s;
                p.slides.push_back(s);
                break;
            default:
                cerr << "wft??? " << key << " -> "  << hex << (int) key << endl;
                throw invalid_argument("illegal key");
        }
    }

    p.sort();
    return input;
}

ostream &operator<<(ostream &output, const pattern &p) {
    output << "chippy";
    for (const evt_note &note: p.notes) {
        output << note;
    }
    for (const evt_slide &slide: p.slides) {
    }
    write_stream(output, KEY_DONE);
    return output;
}

void pattern::sort() {
    ::sort(notes.begin(),  notes.end(),  [](evt_note &e1,  evt_note &e2)  { return e1.start < e2.start; });
    ::sort(slides.begin(), slides.end(), [](evt_slide &e1, evt_slide &e2) { return e1.start < e2.start; });
}


