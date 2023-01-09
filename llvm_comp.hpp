#ifndef LLVM_COMP_H
#define LLVM_COMP_H
#include "hw3_output.hpp"
#include "bp.hpp"
#include <vector>
#include <string>
#include <list>
#include <stack>
#include <iostream>

enum Var_Type
{
    V_INT,
    V_VOID,
    V_BOOL,
    V_BYTE,
    V_STRING,
    UNDEFINED
};

class LLVM_Comp
{
    int curr_reg;

public:
    LLVM_Comp() : curr_reg(0) {}

    string freshVar() { return "%t" + to_string(curr_reg++); }
    int get_curr_reg() { return curr_reg; }
    string makeZext(std::string var_name, std::string cur_size, std::string new_size);
    string whichOP(string op);
    string operationSize(Var_Type type);
};

#endif