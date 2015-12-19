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
#include <typeindex>
#include <sstream>
#include <iomanip>

int expr_id();
string demangle(const string &name);
void print_types(ostream &out, initializer_list<type_index> types);

class expr_context {
    public:
        expr_context() {}
        void write(FILE *out);

        template <typename T, typename ... ARGS>
        void define(const string &name, T (*func)(ARGS...)) {
            stringstream line;
            line << "static " << demangle(typeid(T).name()) 
                 << "(*" << name << ")(";
            print_types(line, {typeid(ARGS)...});
            line << ") = (" << demangle(typeid(T).name()) << "(*)(";
            print_types(line, {typeid(ARGS)...});
            line << ")) " << hex << reinterpret_cast<void*>(func) << ";";
            lines.push_back(line.str());
        }

    private:
        vector<string> lines;
};

template<typename T>
class expr {
    public:
        expr() { handle = 0; func = 0; }
        expr(expr_context &ctx, const string&);
        ~expr();
        T eval() { 
            return func ? func() : 0;
        }

        expr &operator=(expr &&e) {
            this->handle = e.handle;
            this->func = e.func;
            e.handle = 0;
            e.func = 0;
            return *this;
        }

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

    string type_name = demangle(typeid(T).name());

    fflush(0);
    string cmd = "gcc -shared -fPIC -xc -o " + lib_name + " - >/dev/null 2>/dev/null";
    FILE *gcc = popen(cmd.c_str(), "w");

    fprintf(gcc, "#include <stdlib.h>\n");
    ctx.write(gcc);
    fprintf(gcc, "%s %s() { return %s ; }\n", type_name.c_str(), func_name.c_str(), expr.c_str());

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

