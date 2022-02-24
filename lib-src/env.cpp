
#include "env.hpp"
#include "reader.hpp"
#include "data.hpp"
#include <iostream>

Environment::Environment(VM* vm,
                         vector<pair<const char*, PrimLispFn>> prim_pairs) :
    Environment()
{
#define VM_CONS(x, y) vm->makeCell(TypePair, x, y)
    for (auto& pair : prim_pairs) {
        const char* name = pair.first;
        PrimLispFn def = pair.second;

        Cell *prim_name = vm->getSymbol(name); 
        Cell *prim_def = new Cell(TypePrim, (void*)def, (Cell*)name);
        this->frame = VM_CONS(VM_CONS(prim_name, prim_def), this->frame);
    }
}

void Environment::mark() {
    frame->mark();
    if (!this->is_root())
        this->parent->mark();
}

int Environment::count_obj() {
    return this->frame->count_obj()
        + (this->is_root() ? 0 : this->parent->count_obj());
}

Environment *init_environment(VM* vm) {
    auto x = {
        make_pair("list", +[](Cell* args) {
            return args;
        }),
        make_pair("cons", +[](Cell* args) {
            return cons((Cell*)car(args), (Cell*)cadr(args));
        }),
        make_pair("car", +[](Cell* args) { return car((Cell*)car(args));}),
        make_pair("cdr", +[](Cell* args) { return cdr((Cell*)car(args));}),
        make_pair("eq", +[](Cell* args) {
            return to_lisp_bool((Cell*)car(args) == (Cell*)cadr(args));
        }),
        make_pair("atom?", +[](Cell* args) {
            return to_lisp_bool(is_atom((Cell*)car(args)));
        }),
        make_pair("exit", +[](Cell* args) {
            exit(1);
            return nil(); // make type inference happy
        })
    };
    Environment *env = new Environment(vm, x);

#define DEF_PRIM_FN(name, def) ({                                       \
            static Cell prim_name = Cell(TypeSymbol, ((void*)name));            \
            static Cell prim_def = Cell(TypePrim, ((void*)def), ((Cell*)name)); \
            env_add_var_def(&prim_name, &prim_def, env);                \
        })
    
    return env;
}

#define make_bind(var, val) cons(var, val)

Cell *env_add_var_def(Cell *var, Cell *val, Environment *env) {
    // Only calling from eval, and is checked, so redundant
    // ensure(var, TypeSymbol);
    Cell *frame = env->frame;
    // check if at root frame
    if (env->is_root()) {
        /* debuglog("top level def %p\n", env); */
        /* set_car(env, _cons(_cons(var, val), frame)); */
    } else {
        /* debuglog("nested def %p\n", env); */
    }
    env->frame = cons(make_bind(var, val), frame);
    return val;
}

Cell *env_lookup_var(Cell *var, Environment *env) {
    // Same reason as above, in env_add_var_def
    // ensure(var, TypeSymbol);
    /* debuglog("length of env, %p\n", env->type); */

    Cell *pair = assoc(var, env->frame);
    if (!null(pair)) {
        /* debuglog("variable found, %s\n", (char*)var->val); */
        Cell *def = cdr(pair);
        debuglog1("variable found, ");
        debugObj(var, ", ");
        debuglnObj(def);
        return def;
    }
    else if (!env->is_root()) {
        return env_lookup_var(var, env->parent);
    }
    return_error("variable not defined, %s\n", (char*)var->val);
}

Cell *env_set_variable_value(Cell *var, Cell *val, Environment *env) {
    ensure(var, TypeSymbol);

    Cell *pair = assoc(var, env->frame);
    if (!null(pair)) {
        pair->set_cdr(val);
        return val;
    }
    else if (!env->is_root()) {
        return env_set_variable_value(var, val, env->parent);
    }
        
    return_error("variable not defined, %s\n", (char*)var->val);
    // return nil();
}

Cell *make_frame(Cell *arg_syms, Cell *args) {
    Cell front{};
    Cell *frame = &front;

    for (;
         !null(arg_syms);
         arg_syms = cdr(arg_syms),
             args = cdr(args))
    {
        Cell *next = list(
            make_bind(car(arg_syms),
                      car(args)));
        frame->set_cdr(next);
        frame = next;
    }
    return cdr(&front); 
}

Environment *env_extend_stack(Cell *arg_syms, Cell *args, Environment *env) {
    // TODO
    // ensure(arg_syms, TypePair);
    // ensure(args, TypePair);

    // TODO: initialize with, instead of adding manually
    env = env->extend();
    for (;
         !null(arg_syms);
         arg_syms = cdr(arg_syms),
             args = cdr(args))
    {
        env_add_var_def((Cell*)car(arg_syms), (Cell*)car(args), env);
    }
    return env;
}
