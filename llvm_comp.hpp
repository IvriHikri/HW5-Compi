#ifndef LLVM_COMP_H
#define LLVM_COMP_H
#include "hw3_output.hpp"
#include "classes.h"
#include "bp.hpp"
#include <vector>
#include <string>
#include <list>
#include <stack>
#include <iostream>

class LLVM_Comp
{
    int curr_reg;
    int global_reg;

public:
    LLVM_Comp() : curr_reg(0), global_reg(0) {}

    string freshVar() { return "%t" + to_string(curr_reg++); }
    string globalFreshVar() { return "@str" + to_string(global_reg++); }
    int get_curr_reg() { return curr_reg; }
    string makeTruncZext(string var_name, string cur_size, string new_size, string operation);
    string whichOP(string op, Var_Type type);
    string whichRelop(string relop, Var_Type type);
    string operationSize(Var_Type type);
    bool isBoolLiteral(string symbol);
    void CreateBranch(Exp *exp);
    void AddLabelAfterExpression(Exp *exp);
    void AndExp(Exp *exp, Exp *e1, Exp *e2);
    void OrExp(Exp *exp, Exp *e1, Exp *e2);
    void RelopExp(Exp *exp, Exp *e1, Exp *e2, string rel);
    void ExpIfExpElseExp(Exp* exp, Exp* e1, Exp* e2, Exp* e3);
};

static LLVM_Comp comp;

#endif