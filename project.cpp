#include <string>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <fstream>
#include <algorithm>
#include <cstring>


using namespace std;
#include "project.h"

template <typename T>
static T read_stream(istream &input) {
    T value;
    input.read(reinterpret_cast<char*>(&value), sizeof value);
    return value;
}

template <typename T>
static void write_stream(ostream &output, const T value) {
    output.write(reinterpret_cast<const char*>(&value), sizeof value);
}

istream &operator>>(istream &input, evt_note &n) {
    n = read_stream<evt_note>(input);
    return input;
}

ostream &operator<<(ostream &output, const evt_note &note) {
    write_stream(output, note);
    return output;
}

ostream &operator<<(ostream &output, const instrument &i) {
    output << i.volume << " " 
           << i.enabled << " " 
           << (i.enabled ? i.value : "(0f)") << endl
           << (i.name.empty() ? " " : i.name) << endl;
    return output;
}


istream &operator>>(istream &input, instrument &i) {
    input >> i.volume >> i.enabled;
    input.ignore();
    getline(input, i.value);
    getline(input, i.name);
    return input;
}

istream &operator>>(istream &input, sequence &s) {
    s.notes.clear();
    s.natural_length = 0;
    s.ts = read_stream<uint8_t>(input);
    if (s.ts == 0) {
        s.ts = 4;
    }
    s.length        = read_stream<uint16_t>(input);
    uint16_t size   = read_stream<uint16_t>(input);
    while (size--) {
        evt_note n;
        input >> n;
        s.notes.push_back(n);
    }
    s.sort();
    return input;
}

ostream &operator<<(ostream &output, const sequence &s) {
    write_stream(output, s.ts);
    write_stream(output, s.length);
    write_stream(output, (uint16_t) s.notes.size());
    for (const evt_note &note: s.notes) {
        output << note;
    }
    return output;
}

void sequence::sort() {
    ::sort(notes.begin(),  notes.end(),  [](evt_note &e1,  evt_note &e2) {
        return e1.start < e2.start;
    });
}


void sequence::add_note(const evt_note &n) {
    int measure = (n.start + n.length) / ts;
    if (measure >= natural_length) {
        natural_length = measure + 1;
    }
    notes.push_back(n);
}


void sequence::update_natural_length() {
    natural_length = 0;
    for (evt_note &n: notes) {
        int measure = (n.start + n.length) / ts;
        if (measure >= natural_length) {
            natural_length = measure + 1;
        }
    }
}


ostream &operator<<(ostream &output, const project &p) {
    output << "chippy\n"
           << p.name << "\n"
           << p.bpm << "\n";
    for (int i = 0; i < 8; ++i) {
        output << p.instruments[i];
    }
    for (int i = 0; i < 4; ++i) {
        output << p.sequences[i];
    }
    return output;
}

istream &operator>>(istream &input, project &p) {
    string line;
    if (!getline(input, line))      throw invalid_argument("truncated project file");
    if (line != "chippy")           throw invalid_argument("malformed project header");
    if (!getline(input, p.name))    throw invalid_argument("truncated project file");

    input >> p.bpm;
    for (int i = 0; i < 8; ++i) {
        input >> p.instruments[i];
    }
    //input.ignore();
    for (int i = 0; i < 4; ++i) {
        input >> p.sequences[i];
    }
    return input;
}
