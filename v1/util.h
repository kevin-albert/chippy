#ifndef io_h
#define io_h

#include <istream>
#include <ostream>
#include <fstream>

extern ofstream debug;

#define trace(thing) {\
    debug << __func__ << "() ["  << __FILE__ << ":" << __LINE__ << "] " << thing << endl;\
    debug.flush();\
}

const int endian_sample = 1;
#define is_big_endian() ((*(char*)&endian_sample)==0)

template <typename T>
T read_stream(istream &input) {
    T value;
    input.read(reinterpret_cast<char*>(&value), sizeof value);
    return value;
}

template <typename T>
void write_stream(ostream &output, const T value) {
    output.write(reinterpret_cast<const char*>(&value), sizeof value);
}

template <typename T>
T flip_endian(T value) {
    T result = 0;
    for (int i = 0; i < sizeof value; ++i) {
        int shift = (sizeof value - 1 - i) << 3;
        result |= ((value & (0xff << shift)) >> shift) << (i<<3);
    }
    return result;
}

template <typename T>
T to_little_endian(T value) {
    if (is_big_endian()) {
        return flip_endian(value);
    } else {
        return value;
    }
}

template <typename T>
T to_big_endian(T value) {
    if (is_big_endian()) {
        return value;
    } else {
        return flip_endian(value);
    }
}

inline int max(int a, int b) {
    return a > b ? a : b;
}

inline int min(int a, int b) {
    return a < b ? a : b; 
}


#endif
