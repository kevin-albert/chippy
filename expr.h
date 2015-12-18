#ifndef expr_h
#define expr_h

#include <vector>
#include <iostream>
#include <string>
#include <cstdio>
#include <dlfcn.h>
#include <stdexcept>
#include <typeinfo>
#include <cxxabi.h>
#include <sstream>
#include <iomanip>

int expr_id();

class expr_context {
    public:
        expr_context() {}

        template <typename T>
        void add_thing(const string &name, const string &type, T *func) {
            names.push_back(name);
            types.push_back(type);
            values.push_back((void*(*))func);
        }

        void write(FILE *out);
    private:
        vector<const string> names;
        vector<const string> types;
        vector<void*(*)> values;
};

template<typename T>
class expr {
    public:
        expr(expr_context &ctx, const string&);
        ~expr();
        T eval() { return func(); }
    private:
        void *handle;
        T (*func)();
};

template<typename T>
expr<T>::expr(expr_context &ctx, const string &expr) {
    stringstream func_name_str;
    func_name_str << "hack__" << hex << expr_id();
    string func_name(func_name_str.str());
    string lib_name = "/tmp/" + func_name + ".so";
    char *type_name = abi::__cxa_demangle(typeid(T).name(), 0, 0, 0);

    fflush(0);
    string cmd = "gcc -shared -fPIC -xc -o " + lib_name + " -";
    FILE *gcc = popen(cmd.c_str(), "w");

    fprintf(gcc, "#include <math.h>\n");
    ctx.write(gcc);
    fprintf(gcc, "%s %s() { return %s ; }", type_name, func_name.c_str(), expr.c_str());
    free(type_name);

    if (pclose(gcc) != 0) {
        throw invalid_argument("cannot compile expr");
    }

    handle = dlopen(lib_name.c_str(), RTLD_LAZY);
    if (!handle) {
        string err = dlerror();
        throw runtime_error("cannot link compiled expr: " + err); 
    }

    // clear dlerror
    dlerror();
    
    // load the function
    func = (T (*)()) dlsym(handle, func_name.c_str());
    char *dlerr = dlerror();
    if (dlerr) {
        string err = dlerr;
        throw runtime_error("cannot load expr: " + err);
    }
}

template<typename T>
expr<T>::~expr() {
    if (handle) {
        dlclose(handle);
        handle = 0;
    }
}

#endif

