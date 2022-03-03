
#ifndef LIST_HEADER
#define LIST_HEADER

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <string>
#include <vector>
#include <iostream>
#include <functional>

using namespace std;

#define DEBUG

#ifdef DEBUG
#define debuglog1(msg) printf("%-15s: " msg, __func__)
#define debuglog(fmt, ...) printf("%-15s: " fmt, __func__, __VA_ARGS__)
/* #define debuglogln(msg) printf("%-15s: " msg "\n", __func__) */

#define debugObj(cell, msg) (cout << #cell " = " << cell << msg)           
#define debuglnObj(cell) debugObj(cell, "\n")
#else
#define debuglog1(msg)
#define debuglog(...)
#define debugObj(msg1, cell)
#define debuglnObj(cell)
#endif

extern std::string Backtrace(int skip = 1);
#define error(msg, ...) ({                      \
            cout << Backtrace();                \
            printf(msg, __VA_ARGS__);           \
            exit(1);                            \
        });
    
#define ensure(exp, thetype) ({                                         \
            if (exp->type != thetype) {                                 \
                return_error("%s is not of type %d", #exp, thetype);    \
            }})

enum LispType {
    TypeUnknown, // 0
    //
    TypeInt, // 1
    TypeFloat,
    TypeRatio,
    TypeFixNum,
    //
    TypeString, // 5
    TypeSymbol,
    TypePair,
    TypePrim,
    TypeError, // 9
    TypeProcedure
};

// Forward declare
struct Cell;
struct Procedure;
struct Environment;

typedef vector<Cell*> List;

// struct Symbol {
//     const char* val;

//     bool operator ==(Symbol const& sym) { return this->val == sym.val; }
// };

struct Cell {
public:
    LispType type;
    bool in_use;
    void *val;
    Cell *next;

    Cell(LispType type=TypeUnknown, void* val=NULL, Cell* next=nullptr) :
        type(type), val(val), next(next) {}

    Cell* car() {
        if (type == TypePair)
            return (Cell*)val;
        error("CAR used on non pair cell, %d", type);
    }
    Cell* cdr() {
        if (type == TypePair)
            return next;
        error("CDR used on non pair cell, %d", type);
    }

    // void set_car(Cell *val) { this->val = val; }
    void set_cdr(Cell *val) { this->next = val; }

    void mark();
    void free_cell();
    int count_obj();

    friend ostream& operator<<(ostream &out, Cell *exp);
    
    string as_string() {
        if (type == TypeString || type == TypeSymbol)
            return string((char*)(this->val));
        error("trying to access type %d as string", type);
    }
    // const char* as_char_str() { return (const char*)(this->val); }
    // List& as_list() { return ( *)(this->val); }
#define DEF_CONVERTER(name, type)                                   \
    type name () { return *reinterpret_cast<type*>(&this->val); }

    DEF_CONVERTER(as_char_str, char*)
    DEF_CONVERTER(as_procedure, Procedure*)
    DEF_CONVERTER(as_cell, Cell*)
    
#define CONVERT_AS(type) DEF_CONVERTER(as_ ## type, type)

    CONVERT_AS(int)
    CONVERT_AS(float)
    CONVERT_AS(double)
    // CONVERT_AS(List)
};

typedef vector<Cell*> List;

// prim uses next field to store name 
#define prim_name(x) ((char*)x->next)

struct Procedure {
public:
    Cell *param;
    Cell *body;
    Environment *env;
};

// GC code from https://github.com/munificent/lisp2-gc

#define STACK_MAX 256
#define HEAP_SIZE (1024 * 1024)

struct VM {
    List symbols;
    vector<Cell*> heap;
    Environment *root_env;

    void gc();
    void freeAll();
    Cell* newObject();
    Cell* getSymbol(char const* sym);
    Cell* makeCell(LispType type, void* x, Cell* y);
    // Cell* makeCell(LispType type, Cell* x, Cell* y);
    int numObjs() { return heap.size(); }

    VM();
};

#define string_eq(x, y) (strcmp((char *)x, (char *)y) == 0)
#define cell_type(x) ((x)->type)

Cell *nil(void);
VM *getVM(void);

// Cell *make_cell(LispType type, void *data);
// Cell *cons(Cell *x, Cell *y);
#define make_cell(type, data) (getVM()->makeCell(type, data, NULL))
#define cons(x, y) (getVM()->makeCell(TypePair, (void*)x, y))
#define list(x) (cons(x, nil()))

#define lisp_true intern("t")
#define is_bool(x) (null(x) || (x) == lisp_true)

// #define falsep(x) (null(x))
#define to_lisp_bool(x) ((x) ? lisp_true : nil())
#define from_lisp_bool(x) (x == lisp_true ? true : false)

#define intern(x) (getVM()->getSymbol(x))
// Cell *intern(const char *sym);

inline Cell* car(Cell* x) { return x->car(); }
inline Cell* cdr(Cell* x) { return x->cdr(); }
// #define cdr(x)   ((x)->cdr())
#define cddr(x)  (cdr(cdr(x)))
#define cadr(x)  (car(cdr(x)))
#define caddr(x) (car(cdr(cdr(x))))


#define return_error(msg, ...) ({                                       \
            char str[128];                                              \
            sprintf(str, "ERROR: %s, " msg,                             \
                    __func__, __VA_ARGS__);                             \
            return make_cell(TypeError, strdup(str));                   \
        })
/* #define make_error(msg) make_cell(TypeError, (void*)msg) */

#define is_atom(x)   ((x)->next == NULL)

#define is_integer(x)(cell_type(x) == TypeInt)
#define is_float(x)  (cell_type(x) == TypeFloat)
#define is_ratio(x)  (cell_type(x) == TypeRatio)

#define is_string(x) (cell_type(x) == TypeString)
#define is_symbol(x) (cell_type(x) == TypeSymbol)
#define is_pair(x)   (cell_type(x) == TypePair)
#define is_primitive(x) (cell_type(x) == TypePrim)
#define is_error(x)  (cell_type(x) == TypeError)
#define is_procedure(x) (cell_type(x) == TypeProcedure)

typedef Cell *(*PrimLispFn)(Cell*);

#define return_if_find_item_in_list(list_var, lambda) ({                     \
            auto result = find_if(begin(list_var), end(list_var), lambda);   \
            if (result != end(list_var)) { return *result; }            \
        })

// this is more like doRestOfList
#define dolist_cdr(var, list) for (Cell *var = list; !null(var); var = cdr(var))
#define prog1(type, var, ret_exp, body) ({      \
            type var = ret_exp;                 \
            body;                               \
            return var;                         \
        })


// reader
extern Cell *lisp_read(FILE *input);

bool null(Cell *x);
bool is_number(Cell *x);

bool equal(Cell *x, Cell *y);

Cell *make_cCell(int num, ...);

Cell *reverse(Cell *l);

Cell *assoc(Cell *sym, Cell *alist);
int length(Cell *list);

#endif

