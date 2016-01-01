#include <fstream>
#include <mutex>

using namespace std;
#include "io.h"

ofstream debug("/opt/chippy_files/debug.out");
mutex debug_mtx;

