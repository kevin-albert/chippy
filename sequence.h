#ifndef sequence_h
#define sequence_h

#include <iostream>
#include <vector>

/**
 * data class for sequences
 * basic list of events
 */

#define KEY_DONE    'x'
#define EVT_NOTE    '+'
#define EVT_SLIDE   '~'

/**
 * sequence format
 * i1 
 * event...
 * i2 
 * event...
 * i3 
 * event...
 * i4 
 * event...
 *
 * event format
 * note on:     EVT_NOTE    start length note vel 8 bytes
 * end slide:   EVT_SLIDE   start length start_note start_vel end_note end_vel 10 bytes
 */

/*
 * all time units 1/16th note
 */

struct evt_note {
    evt_note() {}
    evt_note(uint8_t instrument, uint32_t start, uint16_t length, uint8_t note, uint8_t vel):
        instrument(instrument),
        start(start),
        length(length),
        note(note),
        vel(vel) {}

    uint8_t instrument;
    uint32_t start;
    uint16_t length;
    uint8_t note;
    uint8_t vel;
};

ostream &operator<<(ostream&, const evt_note&);
istream &operator>>(istream&, evt_note&);

struct evt_slide {
    evt_slide() {}
    evt_slide(uint8_t instrument, uint32_t start, uint16_t length, 
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

ostream &operator<<(ostream&, const evt_slide&);
istream &operator>>(istream&, evt_slide&);

struct sequence {
    vector<evt_note> notes;
    vector<evt_slide> slides;
    void sort();
};

ostream &operator<<(ostream&, const sequence&);
istream &operator>>(istream&, sequence&);

#endif
