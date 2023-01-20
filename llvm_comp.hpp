#ifndef LLVM_COMP_H
#define LLVM_COMP_H
#include <vector>
#include <string>
#include <list>
#include <stack>
#include <iostream>
#include "symbolTable.hpp"

class LLVM_Comp
{
    LLVM_Comp() : curr_reg(0), global_reg(0), stack_for_function(""), cb(CodeBuffer::instance()), sym(SymbolTable::instance()) {}

public:
    int curr_reg;
    int global_reg;
    string stack_for_function;
    CodeBuffer &cb;
    SymbolTable &sym;
    static LLVM_Comp &getInstance() // make SmallShell singleton
    {
        static LLVM_Comp instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    LLVM_Comp(LLVM_Comp const &) = delete;      // disable copy ctor
    void operator=(LLVM_Comp const &) = delete; // disable = operator

    void declareFunc(Type *type, Id *id, Formals *formals);
    void closeFunction(Type *type);
    void callFunc(Call *exp, string func_id, Var_Type retrunType, vector<Exp *> arg_list);
    void printCodeBuffer();

    /* ====================  Conditionals ==================== */
    void CreateBranch(Exp *exp);
    void AddLabelAfterExpression(Exp *exp);
    string DeclareBool(Exp *exp);
    void DecalreBoolArgFunc(Exp *exp);
    void AndExp(Exp *exp, Exp *e1, Exp *e2);
    void OrExp(Exp *exp, Exp *e1, Exp *e2);
    void RelopExp(Exp *exp, Exp *e1, Exp *e2, string rel);
    void BinopExp(Exp *exp, Exp *e1, Exp *e2, string operation);
    void ExpIfExpElseExp(Exp *exp, Exp *e1, Exp *e2, Exp *e3);
    void startIF(Exp *exp);
    void endIF(Exp *exp, Statement *st);
    void startElse(Node *symbol);
    void endElse(Exp *exp, Statement *s1, Node *else_symbol, Statement *s2);
    void start_while();
    void end_while(Exp *exp, Statement *st);

    /* ====================  Helper Functions ==================== */
    string freshVar() { return "%t" + to_string(curr_reg++); }
    string globalFreshVar() { return "@str" + to_string(global_reg++); }
    int get_curr_reg() { return curr_reg; }
    string get_stack_for_function() { return stack_for_function; }
    int emit(string to_emit);
    void emitGlobal(string to_emit);
    string makeTruncZext(string var_name, string cur_size, string new_size, string operation);
    string whichOP(string op, Var_Type type);
    string whichRelop(string relop, Var_Type type);
    string operationSize(Var_Type type);
    bool isBoolLiteral(string symbol);
    void mergeLists(Statement *sts, Statement *st);
    void openGlobalScope();
};

#endif