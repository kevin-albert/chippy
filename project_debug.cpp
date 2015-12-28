#include <iostream>
#include <fstream>
#include <string>

using namespace std;
#include "project.h"

int main(int argc, char **argv) {
    if (argc != 2)  {
        cerr << "usage: " << argv[0] << " project-name\n";
        return 1;
    }
    
    string project_name = argv[1];
    ifstream in("/opt/chippy_files/" + project_name + ".chip");
    if (!in.is_open()) {
        cerr << "unable to open " + project_name << endl;
        return 2;
    }
    project p;
    in >> p;
    cout << "name:          " << p.name 
         << "bpm:           " << p.bpm
         << "instruments:   " << endl;
    for (int i = 0; i < 4; ++i) {
        cout << "   i[" << i << "]: \"" << p.instruments[i].name << "\" " << (p.instruments[i].enabled ? p.instruments[i].value : "ø") << endl;
    }
    cout << "sequences:     " << endl;
    for (int i = 0; i < 4; ++i) {
        cout << "   s[" << i << "]: ts=" << (int) p.sequences[i].ts << ", length: " << p.sequences[i].length << endl;
        for (evt_note &n : p.sequences[i].notes) {
            cout << "        n["
                 <<   "i=" << (int) n.instrument 
                 << ", s=" << (int) n.start 
                 << ", e=" << (int) (n.start+n.length) 
                 << ", n=" << (int) n.start_note << " -> " << (int) n.end_note
                 << ", v=" << (int) n.start_vel << " -> " << (int) n.end_vel
                 << endl;
        }
    }
}
