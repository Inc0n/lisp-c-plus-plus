
#include <stdarg.h>
#include "data.hpp"
#include "env.hpp"
#include "reader.hpp"

/* #define TODO(str) (printf("at %s: %s", __func__, str);) */
#define TODO(str) printf(str);


static Cell sym_nil = Cell(TypeSymbol, NULL);
inline Cell *nil(void) { return &sym_nil; }

static VM *global_vm = new VM();
inline VM *getVM(void) { return global_vm; }

inline bool null(Cell *x) {
    return x == nil() || x == intern("nil");
}

bool is_number(Cell *x) {
    return x->type == TypeInt
        || x->type == TypeFloat
        || x->type == TypeRatio
        || x->type == TypeFixNum;
}

VM::VM() {
    // Creates a new VM with an empty stack and an empty (but allocated) heap.
    heap = List();
    symbols = List();
    root_env = init_environment(this);
}

//

void Cell::free_cell() {
    switch(this->type) {
    case TypeString: {
        delete this;
    } break;
    case TypePair:
        if (!null(this) && !this->in_use) {
            this->car()->free_cell();
            this->cdr()->free_cell();
            delete this;
        }
        break;
    case TypeProcedure: {
        delete this->as_procedure();
        delete this;
    } break;
    case TypeSymbol:
    case TypeInt:
    case TypeFixNum:
    case TypeFloat:
    case TypeRatio:
    default:
        delete this;
        break;
    }
}

// Marks [object] as being reachable and still (potentially) in use.
void Cell::mark() {
    // If already marked, we're done. Check this first to avoid recursing
    // on cycles in the object graph.
    if (this->in_use) return;

    // Any non-zero pointer indicates the object was reached. For no particular
    // reason, we use the object's own address as the marked value.
    this->in_use = true;

    // Recurse into the object's fields.
    if (is_pair(this)) {
        this->car()->mark();
        this->cdr()->mark();
    } else if (is_procedure(this)) {
        this->as_procedure->env
    }
}


// Free memory for all unused objects.
void VM::gc() {
    int len = this->heap.size();
    // Find out which objects are still in use.
// The mark phase of garbage collection. Starting at the roots (in this case,
// just the stack), recursively walks all reachable objects in the VM.
    root_env->mark();
    // free the memories
    auto it = this->heap.begin();
    while(it != this->heap.end()) {
        // If element is even number then delete it
        if((*it)->in_use) {
            it++;
        } else {
            (*it)->free_cell();
            // Due to deletion in loop, iterator became
            // invalidated. So reset the iterator to next item.
            it = this->heap.erase(it);
        }
    }

    printf("%ld live bytes after collection.\n", len - this->heap.size());
}


Cell* VM::newObject() {
    if (this->heap.size() > HEAP_SIZE) {
        this->gc();

        // If there still isn't room after collection, we can't fit it.
        if (this->heap.size() > HEAP_SIZE) {
            perror("Out of memory");
            exit(1);
        }
    }

    Cell* object = new Cell();
    this->heap.push_back(object);
    return object;
}

Cell* VM::makeCell(LispType type, void* data, Cell* y) {
    Cell *_cell = this->newObject();
    debuglog("type=%d, %p\n", type, data);
    /* print_expr(_cell); */
    /* Cell *_cell = calloc(1, sizeof(Cell)); */
    _cell->type = type;
    _cell->val = data;
    _cell->next = y;
    return _cell;
}

//

// Cell *make_cell(LispType type, void *data) {
//     return getVM()->make_cell(type, data, NULL);
// }

// Cell *cons(Cell *x, Cell *y) {
//     Cell *_pair = make_cell(TypePair, x);
//     _pair->next = y;
//     return _pair;
// }

int Cell::count_obj() {
    Cell* cell = this;
    if (null(cell))
        return 0;
    // check procedure before list
    // procedure has cylic reference to env, this is a black hole (segment fault)
    else if (is_atom(cell) || is_primitive(cell) || is_procedure(cell)) {
        return 1;
    }
    else if (is_pair(cell)) {
        return 1
            + cell->car()->count_obj()
            + cell->cdr()->count_obj();
    }
    else {
        printf("<%s: unsupported exp type=%d>", __func__, cell->type);
    }
    return 1;
}

//

Cell *VM::getSymbol(const char *sym) {
    // if (sym == NULL) return symbols;

    /* debuglog("interning symbol %s\n", sym); */
    return_if_find_item_in_list(symbols, [sym](Cell* symbol) {
            return strncmp(sym, symbol->as_char_str(), 32) == 0;
        });
    // dolist_cdr(_pair, symbols) {
    //     /* debuglog("interning symbol, %p, %p|\n", car(_pair), cdr(_pair)); */
    //     if (car(_pair)
    //         && strncmp(sym, (char*)((Cell*)car(_pair))->val, 32) == 0)
    //         return (Cell*)car(_pair);
    // }

    Cell *newSym = new Cell(TypeSymbol, strdup(sym));
    debuglog("creating new symbol %s\n", sym);
    // symbols = cons(newSym, symbols);
    symbols.push_back(newSym);
    return newSym;
}

bool equal(Cell *x, Cell *y) {
    if (x == y) {
        return true;
    }
    if (x->type == y->type) {
        switch(x->type) {
        case TypeString:
        case TypeSymbol:
            return string_eq(x->val, y->val);
        case TypePair:
            return (equal((Cell *)car(x),
                          (Cell *)car(y))
                    && equal(cdr(x),
                             cdr(y)));
        case TypeInt:
        case TypeFixNum:
            return x->as_int() == y->as_int();
        case TypeFloat:
            return x->as_float() == y->as_float();
        case TypeRatio:
            TODO("implement strust of ratio w {denom, detanator}");
        default:
            TODO("should raise error for undefined type?")
            return x->val == y->val;
        }
    }
    return false;
}

//

Cell *assoc(Cell *sym, Cell *alist) {
    dolist_cdr(c, alist) {
        Cell *pair = (Cell*)car(c);
        /* debuglog("env_lookup_var: %p, %d, %s\n", */
        /*          car(pair), ((Cell*)car(pair))->type, var->val); */
        /* print_expr(pair); */
        if (eq((Cell*)car(pair), sym)) {
            return pair;
        }
    }
    return nil();
}

ostream& operator<<(ostream &out, Cell *exp) {
    if (null(exp)) {
        out << "nil";
    }
    else if (is_symbol(exp) || is_string(exp) || is_error(exp)) {
        out << (char *)exp->val;
    }
    // check procedure before list
    else if (is_procedure(exp)) {
        out << "<Proc " << (void *)exp << ">";
    }
    else if (is_pair(exp)) {
        out << "(" << car(exp);
        /* debuglog("print_expr: %s, %d\n", ((Cell*)car(exp))->val, ((Cell*)car(exp))->type); */
        /* debuglog("print_expr: mid %p, %d\n", (exp)->next, exp->type); */
        Cell *e = cdr(exp);
        /* debuglog("print_expr: after %d, %d\n", e->next == NULL, e->type); */
        if (!null(e) && is_atom(e)) {
            // print cons
            cout << " . " << e << ")";
        } else {
            // print normal list
            for (; e && !null(e); e = cdr(e)) {
                cout << " " << car(e);
            }
            cout << ")";
        }
    }
    else if (is_integer(exp)) {
        cout << exp->as_int();
    }
    else if (is_float(exp)) {
        cout << exp->as_float();
    }
    else if (is_primitive(exp)) {
        out << "<Prim " << prim_name(exp) << " " << (void*)exp << ">";
    }
    else {
        out << __func__ << ": unsupported exp type=" << exp->type;
    }
    return out;
}

int length(Cell *list) {
    int acc = 0;
    dolist_cdr(c, list) {
        acc++;
    }
    return acc; 
}
