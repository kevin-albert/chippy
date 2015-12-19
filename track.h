#ifndef track_h
#define track_h

#include <iostream>

struct track {
    int volume {100};
    bool enabled {false};
    string instrument;
};

ostream &operator<<(ostream&, const track&);
istream &operator>>(istream&, track&);

#endif
