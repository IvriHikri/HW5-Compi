#ifndef LLVM_COMP_H
#define LLVM_COMP_H
#include <vector>
#include <string>
#include <list>
#include <stack>
#include <iostream>
#include "symbolTable.hpp"

static int curr_reg = 0;
static int global_reg = 0;
static string stack_for_function = "";
class LLVM_Comp
{
public:
    CodeBuffer &cb;
    SymbolTable &sym;
    
    LLVM_Comp();
    void CreateBranch(Exp *exp);
    void AddLabelAfterExpression(Exp *exp);
    void AndExp(Exp *exp, Exp *e1, Exp *e2);
    void OrExp(Exp *exp, Exp *e1, Exp *e2);
    void RelopExp(Exp *exp, Exp *e1, Exp *e2, string rel);
    void declareFunc(Type *type, Id *id, Formals *formals);
    void closeFunction(Type *type);
    void callFunc(Call *exp, string func_id, Var_Type retrunType, vector<Exp *> arg_list);
    void ExpIfExpElseExp(Exp *exp, Exp *e1, Exp *e2, Exp *e3);
    void printCodeBuffer();
    /* ====================  Helper Functions ==================== */
    string freshVar() { return "%t" + to_string(curr_reg++); }
    string globalFreshVar() { return "@str" + to_string(global_reg++); }
    int get_curr_reg() { return curr_reg; }
    string get_stack_for_function() { return stack_for_function; }
    void emit(string to_emit);
    void emitGlobal(string to_emit);
    string makeTruncZext(string var_name, string cur_size, string new_size, string operation);
    string whichOP(string op, Var_Type type);
    string whichRelop(string relop, Var_Type type);
    string operationSize(Var_Type type);
    bool isBoolLiteral(string symbol);
};

static LLVM_Comp comp;

#endif