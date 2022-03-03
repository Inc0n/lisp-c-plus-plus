

/* The register machine */
#include "data.hpp"

#include <stdexcept>
#include <vector>
#include <string>
#include <iostream>
#include <functional>
#include <sstream>
#include <set>
#include <utility>

using namespace std;

#define error(expr) ({                          \
            std::ostringstream msg;             \
            msg << expr;   \
            throw std::runtime_error(msg.str()); \
        })

// a customized, variant of type Cell
struct Register : Cell {
private:
    void set_val(void* val) {
        if (this->monitorp)
            cout << "assigning " << this->name << " = " << this << endl;
        this->val = val;
    }
    
public:
    const LispType type;
    const string name;
    // void *val = nullptr;
    bool monitorp = false;
    //
    Register(string name, LispType type=TypeUnknown) : type(type), name(name) {}

    template<typename T>
    T as() { return *reinterpret_cast<T*>(&this->val); }
    template<typename T>
    void set_val(T val) { this->set_val((void*)val); }
    // CONVERT_AS(bool)
};

typedef function<void()> CompiledProc;
typedef function<Cell*()> ValueProc;

struct Instruction {
private:
    // Cell *compiledproc;
    string _op;
    Cell *_args;
public:
    CompiledProc compiled_proc;

    // string op() const { return this->_op; }
    string op() const { return _op; }
    Cell* args() const { return _args; }
    // Instruction(string op, Cell* args) : _op(op), exp(args) {}
    Instruction(Cell* exp) :
        _op(exp->car()->as_string()),
        _args(exp->cdr()) {}

    friend ostream& operator<<(ostream &out, Instruction &exp);
};

ostream& operator<<(ostream &out, Instruction &inst) {
    out << inst.op() << "(" << inst.args() << ")";
    return out;
}

struct Label {
    string name;
    Instruction* instructions;
    Label(Cell* exp) : name(exp->as_string()) {};
};


typedef function<Cell*(List)> PrimitiveFn;
struct Operation {
    string name;
    PrimitiveFn fn;
};


#define inst_assignp(x) (x.op() == "assign")
#define inst_testp(x) (x.op() == "test")
#define inst_gotop(x) (x.op() == "goto")
#define inst_branchp(x) (x.op() == "branch")
#define inst_performp(x) (x.op() == "perform")
#define inst_savep(x) (x.op() == "save")
#define inst_restorep(x) (x.op() == "restore")

// TODO: rewrite tagged as function
#define tagged(tag, x) (tag == car(x))

#define is_const_exp(x) (tagged(intern("const"), x))
#define const_exp_val(x) (cadr(x))

#define is_label_exp(x) (tagged(intern("label"), x))
#define label_exp_name(x) (cadr(x)->as_string())

#define is_register_exp(x) (tagged(intern("reg"), x))
#define register_exp_reg(x) (cadr(x))

#define is_operation_exp(x) (tagged(intern("op"), x))


typedef vector<Label> Labels;
typedef vector<Register> Registers;
typedef vector<Operation> Operations;
typedef vector<Instruction> Instructions;
typedef vector<pair<const char*, PrimLispFn>> prim_pairs;

template <typename T>
T *find_named(string name, vector<T> items) {
    auto result = find_if(begin(items), end(items), [name](T item) {
        return item.name == name;
    });
    return result == items.end() ? nullptr : &*result;
}

Register find_reg_error(string name, Registers regs) {
    auto result = find_named<Register>(name, regs);
    if (result == nullptr)
        error("could not find register, " << name);
    return *result;
}

Label find_label_error(string label, Labels labels) {
    auto result = find_named<Label>(label, labels);
    if (result == nullptr)
        error("could not find label, " << label);
    return *result;
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
    return *(this->next--);
}

void Stack::print_stack_stats() {
    printf("total pushes = %d, max-depth = %d\n",
           this->numpushes, this->maxdepth);
}

struct Machine {
protected:
    Register pc;
    Register flag;
    Stack stack;
    Registers regs;
    Labels labels;
    // TODO: fix breakpoints type
    vector<Register> breakpoints;
    Operations ops;
    Instructions inst_seqs;

    void execute_instruction(const Instruction &inst);
    CompiledProc make_inst_exec_proc(Instruction &inst, Labels labels);
    void update_meta(Instructions insts, Labels labels);
    void update_insts(Instructions insts, Labels labels);

    void install_ops(Operations ops);
    void install_inst_seq(Instructions inst_seqs);

    void allocate_reg(string name);

public:
    bool trace_exec;

    Machine();
    Register& find_reg_error(Cell* name);
    Register& find_reg_error(string name);

    void assemble(Cell* controller_text);
    void execute();
    void start(bool trace);
};

Machine::Machine() :
    pc(Register("pc", TypeInt)),
    flag(Register("flag", TypeUnknown))
{
    labels = Labels();
    stack = Stack();
    regs = Registers();
}


Register& Machine::find_reg_error(string name) {
    auto result = find_named<Register>(name, this->regs);
    if (result == nullptr)
        throw std::invalid_argument("could not find register");
    return *result;
}

Register& Machine::find_reg_error(Cell* name) {
    return find_reg_error(name->as_string());
}

void Machine::install_ops(Operations ops) {
    this->ops.insert(this->ops.end(), ops.begin(), ops.end());
}

void Machine::install_inst_seq(vector<Instruction> inst_seqs) {
    this->inst_seqs.assign(inst_seqs.begin(), inst_seqs.end());
    this->pc.set_val<Instruction*>(inst_seqs.data());
}

void Machine::allocate_reg(string name) {
    auto result = find_named<Register>(name, this->regs);
    if (result == nullptr) {
        this->regs.push_back(Register(name));
    } else
        cout << "register " << name << " already exist" << endl;
}

void Machine::execute() {
    auto pc = this->pc.as<Instruction*>();
    if (pc == &*this->inst_seqs.end())
        return ;
    Instruction *inst = pc;
    if (this->trace_exec) {
        // TODO: implement label reverse lookup
        // TODO: implement register values inspection
        cout << inst << "\n";
    }
    inst->compiled_proc();
    this->execute();
}

void Machine::start(bool trace=false) {
    auto old_trace = trace_exec;
    if (trace) trace_exec = trace;

    this->pc.set_val<Instruction*>(this->inst_seqs.data());
    this->execute();
    // restore
    trace_exec = old_trace;
}

// exp

ValueProc make_prim_proc(Cell* exp, Machine* m, Labels labels) {
    if (is_const_exp(exp)) {
        Cell* c = const_exp_val(exp);
        return [&]() { return c; };
    }
    else if (is_label_exp(exp)) {
        // TODO: ensure as_string works
        string name = label_exp_name(exp);
        Label label = find_label_error(name, labels);
        return [&]() { return (Cell*)label.instructions; };
    }
    else if (is_register_exp(exp)) {
        Register &reg = m->find_reg_error(register_exp_reg(exp));
        // TODO: type check reg.type
        return [&]() { return reg.as<Cell*>(); };
    }
    else error("Assemble: unknown primitive expression, " << exp);
}

// template <typename T>
List as_list(Cell* list) {
    auto acc = List();
    dolist_cdr(l, list) {
        acc.push_back(car(l));
    }
    return acc;
}

template <typename T_in, typename T_out>
vector<T_out> map(vector<T_in> l, function<T_out(T_in)> f) {
    vector<T_out> out;
    out.reserve(l.size());
    transform(l.begin(), l.end(), back_inserter(out), f);
    return out;
}

Cell *operation_exp_op(Cell* exp) { return cadr(car(exp)); }
Cell *operation_exp_operands(Cell* exp) { return cdr(exp); }
ValueProc make_operation_proc(Cell* exp, Machine* m,
                              Labels labels, Operations ops) {
    auto op = find_named<Operation>(operation_exp_op(exp)->as_string(), ops);
    if (op == nullptr)
        error("assemble: unknown operation " << exp);
    auto op_fn = op->fn;
    auto arg_procs = map<Cell*, ValueProc>(
        as_list(operation_exp_operands(exp)),
        [&](Cell* exp) -> ValueProc {
            return make_prim_proc(exp, m, labels);
        });
    return [&]() {
        auto args = map<ValueProc, Cell*>(
            arg_procs,
            [](ValueProc proc) { return proc(); }
            );
        return op_fn(args);
    };
}

#define advance_pc(pc) (pc.set_val<Instruction*>(pc.as<Instruction*>()+1))
#define assign_reg_name(x) ((x)->car())
CompiledProc make_assign(Instruction &inst, Machine* m,
                         Labels labels, Operations ops, Register &pc)
{
    auto reg_name = assign_reg_name(inst.args());
    auto assign_val_exp = inst.args()->cdr();

    Register &reg = m->find_reg_error(reg_name->as_string());
    auto value_proc = is_operation_exp(assign_val_exp)
        ? make_operation_proc(assign_val_exp, m, labels, ops)
        : make_prim_proc(assign_val_exp->car(), m, labels);

    return [&]() {
        reg.set_val<Cell*>(value_proc());
        advance_pc(pc);
    };
}


CompiledProc make_test(Instruction &inst,
                       Machine* m, Labels labels, Operations ops,
                       Register& pc, Register &flag)
{
    auto condition = inst.args();
    if (!is_operation_exp(condition)) 
        error("Assemble: bad test expression, " << inst);
    auto cond_proc = make_operation_proc(condition, m, labels, ops);
    return [&]() {
        flag.set_val<Cell*>(cond_proc());
        advance_pc(pc);
    };
}


CompiledProc make_branch(Instruction &inst,
                         Machine* m, Labels labels, Register& pc, Register &flag)
{
    auto destination = inst.args()->car();
    if (is_label_exp(destination)){
        string name = label_exp_name(destination);
        auto label = find_label_error(name, labels);
        return [&]() {
            if (from_lisp_bool(flag.as_cell()))
                pc.set_val<Instruction*>(label.instructions);
            else advance_pc(pc);
        };
    }
    else error("Assemble: bad branch instruction, " << inst);
}

#define goto_destination(x) ((x)->car())
CompiledProc make_goto(Instruction &inst, Machine* m, Labels labels, Register& pc)
{
    auto destination = goto_destination(inst.args());
    if (is_label_exp(destination)) {
        auto label_name = label_exp_name(destination);
        auto label = find_label_error(label_name, labels);
        return [&]() {
            pc.set_val<Instruction*>(label.instructions);
        };
    } 
    else if (is_register_exp(destination)) {
        auto reg = m->find_reg_error(register_exp_reg(destination));
        return [&]() {
            pc.set_val<Instruction*>((Instruction*)reg.val);
        };
    } 
    else error("Assemble: bad goto instruction, " << inst);
}


CompiledProc make_perform(Instruction &inst, Machine* m,
                          Labels labels, Operations ops, Register& pc)
{
    auto action = inst.args();
    
    if (is_operation_exp(action)) {
        auto action_proc = make_operation_proc(action, m, labels, ops);
        return [&]() {
            action_proc();
            advance_pc(pc);
        };
    }
    else error("Assemble: bad goto instruction, " << inst);
}

CompiledProc make_save(Instruction &inst,
                       Machine* m, Stack &stack, Labels labels, Register& pc)
{
    auto exp = inst.args()->car();
    if (is_label_exp(exp)) {
        auto label = find_label_error(label_exp_name(exp), labels);
        return [&]() {
            stack.push(label.instructions);
            advance_pc(pc);
        };
    } 
    else if (is_register_exp(exp)) {
        auto reg = m->find_reg_error(register_exp_reg(exp));
        return [&]() {
            stack.push(reg.val);
            advance_pc(pc);
        };
    } 
    else error("Assemble: bad save argument, " << inst);
}

#define stack_reg_name(x) (register_exp_reg((x)->car()))
CompiledProc make_restore(Instruction &inst,
                          Machine* m, Stack &stack, Register& pc)
{
    auto exp = inst.args()->car();
    if (is_register_exp(exp)) {
        auto reg = m->find_reg_error(register_exp_reg(exp));
        return [&]() {
            reg.set_val<Cell*>((Cell*)stack.pop());
            advance_pc(pc);
        };
    }
    else error("Assemble: bad save argument, " << inst);
}

CompiledProc Machine::make_inst_exec_proc(Instruction &inst, Labels labels) {
    if (inst_assignp(inst))
        return make_assign(inst, this, labels, this->ops, pc);
    else if (inst_testp(inst))
        return make_test(inst, this, labels, this->ops, pc, flag);
    else if (inst_branchp(inst))
        return make_branch(inst, this, labels, pc, flag);
    else if (inst_gotop(inst))
        return make_goto(inst, this, labels, pc);
    else if (inst_performp(inst))
        return make_perform(inst, this, labels, ops, pc);
    else if (inst_savep(inst))
        return make_save(inst, this, this->stack, labels, pc);
    else if (inst_restorep(inst))
        return make_restore(inst, this, this->stack, pc);
    else
        error("received unknown instruction, " << inst);
}


set<string> extract_regs_meta(Instructions insts) {
    set<string> acc{};
    for (auto &inst : insts) {
        if (inst_assignp(inst))
            acc.insert(assign_reg_name(inst.args())->as_string());
        else if (inst_gotop(inst) || inst_savep(inst) || inst_restorep(inst))  {
            auto dest = inst.args()->car();
            if (is_register_exp(dest))
                acc.insert(register_exp_reg(dest)->as_string());
        }
    }
    return acc;
}

void Machine::update_meta(Instructions insts, Labels labels) {
    this->labels.assign(labels.begin(), labels.end());
    auto regs = extract_regs_meta(insts);
    for (auto reg_name: regs)
        allocate_reg(reg_name);
    // this->regs_srcs.assign()
}

void Machine::update_insts(Instructions insts, Labels labels) {
    for (auto &inst : insts) {
        inst.compiled_proc = this->make_inst_exec_proc(inst, labels);
    }
}

void extract_labels(Cell* text,
                    function<void(Instructions insts, Labels labels)> handler)
{
    if (null(text))
        return handler(Instructions(), Labels());
    return extract_labels(
        text->cdr(),
        [&](Instructions insts, Labels labels) {
            Cell* next_inst = text->car();
            if (is_symbol(next_inst)) {
                if (find_named<Label>(next_inst->as_string(), labels) != nullptr)
                    error("extract-labels: multiple label "
                          << next_inst
                          << " defined");
                else
                    labels.push_back(Label(next_inst));
            }
            else
                insts.push_back(Instruction(next_inst));
            handler(insts, labels);
        });
}

void Machine::assemble(Cell* controller_text) {
    extract_labels(
        controller_text,
        [&](Instructions insts, Labels labels) {
            this->update_meta(insts, labels);
            this->update_insts(insts, labels);
        });
}