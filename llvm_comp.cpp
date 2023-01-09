#include "llvm_comp.hpp"

string LLVM_Comp::makeZext(std::string var_name, std::string cur_size, std::string new_size)
{
    string new_var = freshVar();
    string to_emit = new_var + " = zext " + cur_size + " " + var_name + " to " + new_size;
    cb.emit(to_emit);
    return new_var;
}

string LLVM_Comp::whichOP(string op)
{
    if(op.compare("+") == 0)
    {
        return "add";
    }
    if (op.compare("-") == 0)
    {
        return "sub";
    }
    if (op.compare("*") == 0)
    {
        return "mul";
    }
    if (op.compare("/") == 0)
    {
        return "div";
    }

    return "error";
}

string LLVM_Comp::operationSize(Var_Type type)
{
    switch (type)
    {
    case V_INT:
        return "i32";
    case V_BOOL:
        return "i1";
    case V_STRING:
        return "i8*";
    case V_BYTE:
        return "i8";
    case V_VOID:
        return "void";
    default:
        return "error";
    }
}