#ifndef mem_tcc_inc
#error "include mem.h, not mem.tcc"
#endif

#ifndef mem_tcc
#define mem_tcc

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <string>
#include <stdexcept>

#include "util/util.h"

//
// Allocate + Deallocate
//

template <class T>
Mem::Array<T> Mem::makeArray(const string &projectName, const string &arrayName) {
    return Array<T>(openMemFile(projectName, "mem/a", arrayName));
}

template <class T>
void Mem::closeArray(Mem::Array<T> &array) {
    Mem::unmap(array.data, sizeof array.size + array.capacity * sizeof(T));
    close(array.fd);
}

template <class T>
Mem::Instance<T> Mem::makeInstance(const string &projectName, const string &instanceName) {
    return Instance<T>(openMemFile(projectName, "mem/i", instanceName));
}

template <class T>
void Mem::closeInstance(Mem::Instance<T> &instance) {
    Mem::unmap(instance.data, sizeof(T));
    close(instance.fd);
}

//
// File mapping
//

void *Mem::map(const int fd, const size_t size) {
    if (lseek(fd, size-1, SEEK_SET) == -1) {
        fail("lseek failed");
    }

    if (write(fd, "", 1) != 1) {
        fail("write failed");
    }

    void *data = mmap(nullptr, size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED, fd, 0);
    if (!data) {
        fail("mmap failed");
    }

    return data;
}

void *Mem::remap(const int fd, const size_t size, void *data, const size_t newSize) {
    void *newData = Mem::map(fd, newSize);
    if (data) {
        Mem::unmap(data, size);
    }
    return newData;
}

void Mem::unmap(void *data, const size_t size) {
    ::munmap(data, size);
}

int Mem::openMemFile(const string &projectName, const string &type, const string &memName) {
    string dir = Util::makeProjectDir(projectName, type);
    string filename = dir + "/" + memName;
    int fd = open(filename.c_str(), O_RDWR | O_CREAT, (mode_t) 0600);
    if (fd == -1) {
        fail("open failed");
    }

    return fd;
}

//
// Array operations
//

#include <iostream>

template<class T>
Mem::Array<T>::Array(const int fd):
        fd(fd)
{
    struct stat st;
    if (fstat(fd, &st)) {
        fail("stat failed");
    }

    if (st.st_size < 4) {
        capacity = 10;
        size = 0;
        data = (size_t*) Mem::map(fd, sizeof size + 10 * sizeof(T));
        data[0] = size;
    } else {
        capacity = (st.st_size - sizeof size) / sizeof(T);
        data = (size_t*) Mem::map(fd, sizeof size + 10 * sizeof(T));
        size = data[0];
    }
}

template <class T>
T &Mem::Array<T>::operator[](const size_t i) {
    if (i >= capacity) {
        size_t newCapacity = capacity < 10 ? 10 : capacity;
        while (newCapacity <= i) {
            newCapacity *= 1.5;
        }
        expand(newCapacity);
    }

    if (i >= size) {
        size = i + 1;
        data[0] = size;
    }

    return ((T*)(data + sizeof size))[i];
}

template <class T>
void Mem::Array<T>::expand(const size_t newCapacity) {
    data = (size_t*) remap(fd, sizeof size + capacity * sizeof(T), data, sizeof size + newCapacity * sizeof(T));
    capacity = newCapacity;
}


//
// Instance operations
//

template <class T>
Mem::Instance<T>::Instance(const int fd):
        fd(fd)
{
    data = (T*) Mem::map(fd, sizeof(T));
}

template <class T>
T &Mem::Instance<T>::operator*() {
    return *data;
}

template <class T>
T *Mem::Instance<T>::operator->() {
    return data;
}

template <class T>
Mem::Instance<T> &Mem::Instance<T>::operator=(const T &thing) {
    *data = thing;
    return *this;
}

#endif
