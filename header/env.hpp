#ifndef ENV_HEADER
#define ENV_HEADER

#include <cstddef>
#include <iostream>
#include "data.hpp"

// struct Bind {
//     Cell* var;
//     Cell* val;
// };

struct Environment {
    Environment(Environment* parent = nullptr) {
        this->parent = parent;
        this->frame = nil();
    }
    Environment(VM* vm, vector<pair<const char*, PrimLispFn>> prim_pairs);

    Cell* operator [](Cell* const sym);
    Cell* add(Cell* const sym, Cell* val);

public:
    Environment* parent;
    Cell* frame;

    void mark();
    int count_obj();
    bool is_root() { return this->parent == nullptr; }
    Environment* extend() { return new Environment(this); }
};

/* typedef struct { */
/*     Cell *frames; */
/*     Cell *root; */
/* } Environment; */

Environment *init_environment(VM* vm);
Cell *env_add_var_def(Cell *var, Cell *val, Environment *env);
Cell *env_lookup_var(Cell *var, Environment *env);
Cell *env_set_variable_value(Cell *var, Cell *val, Environment *env);
Environment *env_extend_stack(Cell *arg_syms, Cell *args, Environment *env);

#endif

