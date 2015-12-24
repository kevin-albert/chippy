#include <iomanip>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>

using namespace std;
#include "sequence.h"


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
            read_stream<uint8_t> (input),   // start_note
            read_stream<uint8_t> (input),   // start_vel
            read_stream<uint8_t> (input),   // end_note
            read_stream<uint8_t> (input));  // end_vel
    return input;
}

ostream &operator<<(ostream &output, const evt_note &note) {
    write_stream(output, (char) EVT_NOTE);
    write_stream(output, note.instrument);
    write_stream(output, note.start);
    write_stream(output, note.length);
    write_stream(output, note.start_note);
    write_stream(output, note.start_vel);
    write_stream(output, note.end_note);
    write_stream(output, note.end_vel);
    return output;
}

istream &operator>>(istream &input, sequence &s) {
    s.notes.clear();

    char header[7];
    input.read(header, 6);
    header[6] = '\0';
    if (strcmp(header, "chippy")) {
        throw invalid_argument("invalid header");
    }

    char key;
    while (input.get(key) && key != KEY_DONE) {
        switch (key) {
            case KEY_TS:
                s.ts = read_stream<uint8_t>(input);
                break;
            case KEY_LENGTH:
                s.length = read_stream<uint16_t>(input);
                break;
            case EVT_NOTE:
                {
                    evt_note n;
                    input >> n;
                    s.notes.push_back(n);
                }
                break;
            default:
                throw invalid_argument("illegal key");
        }
    }

    s.sort();
    return input;
}

ostream &operator<<(ostream &output, const sequence &p) {
    output << "chippy";
    write_stream(output, (char) KEY_TS);
    write_stream(output, p.ts);
    write_stream(output, (char) KEY_LENGTH);
    write_stream(output, p.length);
    for (const evt_note &note: p.notes) {
        output << note;
    }
    return output;
}

void sequence::sort() {
    ::sort(notes.begin(),  notes.end(),  [](evt_note &e1,  evt_note &e2)  { return e1.start < e2.start; });
}


