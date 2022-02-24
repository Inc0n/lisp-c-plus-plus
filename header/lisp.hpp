
#ifndef LISP_HEADER
#define LISP_HEADER

#include "data.hpp"
#include "env.hpp"

Cell *eval(Cell *x, Environment *env);
Cell *apply(Cell *func, Cell *args);

#endif

