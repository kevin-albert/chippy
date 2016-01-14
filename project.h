#ifndef project_h
#define project_h

#include <string>
#include <iostream>
#include <vector>


#define NUM_INSTRUMENTS 8
#define NUM_SEQUENCES 15

#define PAN_LEFT ((uint8_t) 0x00)
#define PAN_RIGHT ((uint8_t) 0xff)
#define PAN_CENTER ((uint8_t) 0x80)

/*
 * all time units 1/16th note
 */

struct evt_note {
    evt_note() {}
    uint8_t instrument;
    uint32_t start;
    uint16_t length;
    uint8_t start_note;
    uint8_t start_vel;
    uint8_t end_note;
    uint8_t end_vel;
    uint8_t pan {PAN_CENTER};
};

ostream &operator<<(ostream&, const evt_note&);
istream &operator>>(istream&, evt_note&);

struct sequence {
    uint8_t ts {4};
    uint16_t length {4};
    uint16_t natural_length {0};
    vector<evt_note> notes;
    void sort();
    void add_note(const evt_note&);
    void update_natural_length();
};


ostream &operator<<(ostream&, const sequence&);
istream &operator>>(istream&, sequence&);

struct instrument {
    int volume {100};
    bool enabled {false};
    string value;
    string name;
};

ostream &operator<<(ostream&, const instrument&);
istream &operator>>(istream&, instrument&);

struct project {
    string name {"untitled"};
    int bpm {120};
    int volume {50};
    instrument instruments[NUM_INSTRUMENTS];
    sequence sequences[NUM_SEQUENCES];
};

ostream &operator<<(ostream&, const project&);
istream &operator>>(istream&, project&);

#endif
