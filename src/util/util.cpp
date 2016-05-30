#include <cstdlib>

using namespace std;

#include "util.h"

namespace Util {

    string makeProjectDir(const string &project, const string &ns) {
        if (ns == "") {
            fail("namespace cannot be empty");
        }
        string dir = string(getenv("HOME")) + "/.chippy/" + project + "/" + ns;
        string cmd = "mkdir -p " + dir;
        if (system(cmd.c_str())) {
            fail("mkdir failed");
        }

        return dir;
    }
}
