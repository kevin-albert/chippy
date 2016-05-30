#ifndef util_h
#define util_h

#include <sstream>
#include <string>
#include <stdexcept>

#define fail(msg) {\
    stringstream what;\
    what << __LINE__ << ":" << __FILE__ << " - " << __PRETTY_FUNCTION__ << ": " << msg;\
    throw runtime_error(what.str());\
}

namespace Util {
    string makeProjectDir(const string &project, const string &ns);
}

#endif
