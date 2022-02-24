

/* The register machine */
#include "data.h"

#include <stdexcept>
#include <vector>
#include <string>
#include <iostream>
#include <functional>
// #include <bool>

using namespace std;

// a customized, variant of type Cell
struct Register {
private:
    void *val;
    
public:
    const string name;
    const LispType type;
    bool monitorp;
    void set_val(void* val);
};

void Register::set_val(void* val) {
    if (this->monitorp)
        cout << "assigning " << this->name << " = " << val << endl;
    this->val = val;
}

typedef function<void()> CompiledProc;
typedef function<Cell*()> ValueProc;

struct Label {
    const string name;
    const Cell *instructions;
};

struct Instruction {
private:
    CompiledProc compiled_proc;
    // Cell *compiledproc;
    Cell *args;
public:
    const string op;
    Instruction(string op, Cell* args);
};

#define inst_assignp(x) (x.op == "assign")
#define inst_testp(x) (x.op == "test")
#define inst_gotop(x) (x.op == "goto")
#define inst_branchp(x) (x.op == "branch")
#define inst_performp(x) (x.op == "perform")
#define inst_savep(x) (x.op == "save")
#define inst_restorep(x) (x.op == "restore")

// TODO: rewrite tagged as function
#define tagged(tag, x) (tag == car(x))

#define is_const_exp(x) (tagged(intern("const"), x))
#define const_exp_val(x) (cadr(x))

#define is_label_exp(x) (tagged(intern("label"), x))
#define label_exp_name(x) (cadr(x))

#define is_register_exp(x) (tagged(intern("reg"), x))
#define register_exp_reg(x) (cadr(x))

#define is_operation_exp(x) (tagged(intern("op"), x))

template <typename T>
T find_named(string name, vector<T> items) {
    return find_if(begin(items), end(items), [](T item) {
        return item.name == name;
    });
}

Register find_reg_error(string name, vector<Register> regs) {
    auto result = find_named<Register>(label, labels);
    if (result == end(labels))
        throw std::invalid_argument("could not find register");
    return *result;
}

Label find_label_error(string label, vector<Label> labels) {
    auto result = find_named<Label>(label, labels);
    if (result == end(labels))
        throw std::invalid_argument("could not find label");
    return *result;
}

ValueProc make_operation_proc(Cell* exp, Machine machine, labels, ops) {
    Cell *operation_exp_op(exp) { return cadr(car(exp)); }
    Cell *operation_exp_operands(exp) { return cdr(exp); }
}

ValueProc make_prim_proc(Cell* exp, Machine machine, vector<Label> labels, ops) {
    if (is_const_exp(exp)) {
        Cell* c = const_exp_val(exp);
        return [&c]() { return c; }
    }
    else if (is_label_exp(exp)) {
        Label label = find_label_error(exp, labels);
        return [&label]() { return label.instructions; }
    }
    else if (is_register_exp(exp)) {
        Cell *name = register_exp_reg(exp);
        const Register &reg = machine->find_reg_error(exp, name);
        return [&reg]() { return reg.val; }
    }
    else
        throw std::invalid_argument("received unknown primitive expression");
}

#define STACK_MAX_SIZE 256

struct Stack {
private:
    int maxdepth;
    int numpushes;
    int currentdepth;
    void *_stack[STACK_MAX_SIZE];
    void **next;

public:
    Stack();
    void push(void *val);
    void* pop();
    void print_stack_stats();
};

Stack::Stack() {
    this->maxdepth = 0;
    this->numpushes = 0;
    this->currentdepth = 0;
    this->next = this->_stack;
}

void Stack::push(void *val) {
    this->numpushes++; 
    this->currentdepth++; 
    if (this->maxdepth < this->currentdepth) 
        this->maxdepth = this->currentdepth;
    // actual push
    *(this->next) = val;
    this->next++;
}

void* Stack::pop() {
    return this->next--;
}

void Stack::print_stack_stats() {
    printf("total pushes = %d, max-depth = %d\n",
           this->numpushes, this->maxdepth);
}

struct Machine {
private:
    Register pc;
    Register flag;
    Stack stack;
    vector<Register> regs;
    vector<Register> breakpoints;
    vector<Cell*> ops;
    vector<Instruction> inst_seqs;

    void execute_instruction(const Instruction &inst);

public:
    bool trace_exec;
    Machine();
    void install_ops(vector<Cell*> ops);
    void install_inst_seq(vector<Cell*> inst_seqs);
    void allocate_reg(Register reg);
    Register& find_reg(string name);

    void execute();
};

Machine::Machine() {
    

}

void Machine::install_ops(vector<Cell*> ops) {
    // TODO: vector join
}

void Machine::install_inst_seq(vector<Cell*> inst_seqs) {
    this->inst_seqs = inst_seqs;
    this->pc.set_val(0);
}

void Machine::allocate_reg(Register reg) {
    // TODO: 
}

Register& Machine::find_reg(string name) {
    // TODO:
}

void Machine::execute() {
    if (this->pc == this->inst_seqs.len())
        return ;
    Instruction inst = this->inst_seqs[this->pc];
    if (this->trace_exec) {
        
    }
    this->execute();
}

CompiledProc make_assign(const Instruction &inst, Machine m) {
    function<void*()> value_proc = 
    return []() {
        reg.set_val()
        return a < b;
    }
}

CompiledProc Machine::compile_instruction(const Instruction &inst) {
    if (inst_assignp(inst)) make_assign(inst, this, labels)
    else if (inst_testp(inst)) {}
    else if (inst_branchp(inst)) {}
    else if (inst_gotop(inst)) {}
    else if (inst_performp(inst)) {}
    else if (inst_savep(inst)) {}
    else if (inst_restorep(inst)) {}
    else {
        throw std::invalid_argument("received unknown instruction");
    }
}

    // if (inst_assignp(inst)) {}
    // else if (inst_testp(inst)) {}
    // else if (inst_branchp(inst)) {}
    // else if (inst_gotop(inst)) {}
    // else if (inst_performp(inst)) {}
    // else if (inst_savep(inst)) {}
    // else if (inst_restorep(inst)) {}
    // else throw std::invalid_argument("received unknown instruction");
    