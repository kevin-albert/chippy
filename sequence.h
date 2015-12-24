#ifndef sequence_h
#define sequence_h

#include <iostream>
#include <vector>

/**
 * data class for sequences
 * basic list of events
 */

#define KEY_DONE    'x' // no more input
#define KEY_TS      't' // quarter notes per measure
#define KEY_LENGTH  'l' // length in measures
#define EVT_NOTE    '+' // note event

/*
 * all time units 1/16th note
 */

struct evt_note {
    evt_note() {}
    evt_note(uint8_t instrument, uint32_t start, uint16_t length, 
             uint8_t start_note, uint8_t start_vel, uint8_t end_note, uint8_t end_vel):
        instrument(instrument),
        start(start),
        length(length),
        start_note(start_note),
        start_vel(start_vel),
        end_note(end_note),
        end_vel(end_vel) {}

    uint8_t instrument;
    uint32_t start;
    uint16_t length;
    uint8_t start_note;
    uint8_t start_vel;
    uint8_t end_note;
    uint8_t end_vel;
};

ostream &operator<<(ostream&, const evt_note&);
istream &operator>>(istream&, evt_note&);

struct sequence {
    uint8_t ts {4};
    uint16_t length {4};
    vector<evt_note> notes;
    void sort();
};


ostream &operator<<(ostream&, const sequence&);
istream &operator>>(istream&, sequence&);

#endif
