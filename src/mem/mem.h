#ifndef mem_h
#define mem_h

#include <string>

using namespace std;

namespace Mem {

    template <class T> class Array;
    template <class T> class Instance;

    template <class T>
    Array<T> makeArray(const string &projectName, const string &arrayName);

    template <class T>
    void closeArray(Array<T> &array);

    template <class T>
    Instance<T> makeInstance(const string &projectName, const string &instanceName);

    template <class T>
    void closeInstance(Instance<T> &instance);

    int openMemFile(const string &projectName, const string &type, const string &memName);

    void *map(const int fd, const size_t size);
    void *remap(const int fd, const size_t size, void *data, const size_t newSize);
    void unmap(void *data, const size_t size);

    template <class T>
    class Array {
        friend void Mem::closeArray<T>(Array<T>&);
        public:
            Array(const int fd);
            size_t size {0};
            size_t capacity {0};

            T &operator[](const size_t i);

        private:
            size_t *data;
            const int fd;
            void expand(const size_t newCapacity);
    };


    template <class T>
    class Instance {
        friend void Mem::closeInstance<T>(Instance<T>&);
        public:
            Instance(const int fd);
            T &operator*();
            T *operator->();
            Instance &operator=(const T &thing);
        private:
            T *data;
            const int fd;
    };
}

#define mem_tcc_inc
#include "mem.tcc"
#undef mem_tcc_inc

#endif
