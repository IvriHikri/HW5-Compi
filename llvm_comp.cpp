#include "llvm_comp.hpp"
#include "hw3_output.hpp"
#include "bp.hpp"
#include "symbolTable.hpp"

LLVM_Comp::LLVM_Comp() : cb(CodeBuffer::instance()), sym(SymbolTable::instance()), curr_reg(0), global_reg(0), stack_for_function("")
{
}

void LLVM_Comp::emit(string to_emit)
{
    cb.emit(to_emit);
}

void LLVM_Comp::emitGlobal(string to_emit)
{
    cb.emitGlobal(to_emit);
}

string LLVM_Comp::whichOP(string op, Var_Type type)
{
    if (op.compare("+") == 0)
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
        return type == V_BYTE ? "udiv" : "sdiv";
    }

    return "errorOP";
}

string LLVM_Comp::whichRelop(string relop, Var_Type type)
{
    string op;
    if (relop.compare("==") == 0)
    {
        return "eq";
    }

    if (relop.compare("!=") == 0)
    {
        return "ne";
    }
    else if (relop.compare(">") == 0)
    {
        op = "gt";
    }
    else if (relop.compare(">=") == 0)
    {
        op = "ge";
    }
    else if (relop.compare("<") == 0)
    {
        op = "lt";
    }
    else if (relop.compare("<=") == 0)
    {
        op = "le";
    }
    else
    {
        return "errorRelop";
    }

    return type == V_BYTE ? "u" + op : "s" + op;
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
        return "errorOPsize";
    }
}

bool LLVM_Comp::isBoolLiteral(string symbol)
{
    if (symbol.compare("true") == 0 || symbol.compare("false") == 0)
    {
        return true;
    }

    return false;
}

void LLVM_Comp::CreateBranch(Exp *exp)
{
    string to_emit = "br i1 " + exp->var_name + ", label @ , label @";
    int loc = cb.emit(to_emit);
    exp->truelist = cb.merge(exp->truelist, cb.makelist({loc, FIRST}));
    exp->falselist = cb.merge(exp->falselist, cb.makelist({loc, SECOND}));
}

void LLVM_Comp::AddLabelAfterExpression(Exp *exp)
{
    exp->label = cb.genLabel();
}

void LLVM_Comp::AndExp(Exp *exp, Exp *e1, Exp *e2)
{
    cb.bpatch(e1->truelist, e1->label);
    CreateBranch(e2);
    AddLabelAfterExpression(e2);
    exp->var_name = e2->var_name;
    exp->truelist = e2->truelist;
    exp->falselist = cb.merge(e1->falselist, e2->falselist);
}

void LLVM_Comp::OrExp(Exp *exp, Exp *e1, Exp *e2)
{
    cb.bpatch(e1->falselist, e1->label);
    CreateBranch(e2);
    AddLabelAfterExpression(e2);
    exp->var_name = e2->var_name;
    exp->falselist = e2->falselist;
    exp->truelist = cb.merge(e1->truelist, e2->truelist);
}

void LLVM_Comp::RelopExp(Exp *exp, Exp *e1, Exp *e2, string rel)
{
    Var_Type type = (e1->type == V_BYTE && e2->type == V_BYTE) ? V_BYTE : V_INT;
    string relop = whichRelop(rel, type);

    string var_name1 = e1->var_name;
    string var_name2 = e2->var_name;

    if (e1->type != type)
    {
        var_name1 = comp.makeTruncZext(var_name1, operationSize(e1->type), operationSize(type), "zext");
    }
    if (e2->type != type)
    {
        var_name2 = comp.makeTruncZext(var_name2, operationSize(e1->type), operationSize(type), "zext");
    }

    exp->var_name = freshVar();
    string to_emit = exp->var_name + "= icmp " + relop + " " + operationSize(type) + " " + var_name1 + ", " + var_name2;
    cb.emit(to_emit);
}

string LLVM_Comp::makeTruncZext(std::string var_name, std::string cur_size, std::string new_size, string operation)
{
    string new_var = freshVar();
    string to_emit = new_var + " = " + operation + " " + cur_size + " " + var_name + " to " + new_size;
    cb.emit(to_emit);
    return new_var;
}

void LLVM_Comp::declareFunc(Type *type, Id *id, Formals *formals)
{
    sym.declareFunction(type->type, id, formals);
    string code = "define " + operationSize(type->type) + " @" + id->value + "(";
    if (!formals->declaration.empty())
    {
        for (FormalDecl *f : formals->declaration)
        {
            code += operationSize(f->type) + ", ";
        }

        code.erase(code.size() - 2);
    }
    code += ") {";
    cb.emit(code);

    this->stack_for_function = this->freshVar() + "_" + sym.currentFunction;
    code = this->stack_for_function + " = alloca i32, i32 50";
    cb.emit(code);
}

void LLVM_Comp::closeFunction(Type *type)
{
    string code = "ret " + operationSize(type->type);
    if (type->type != V_VOID)
    {
        code += " 0";
    }
    cb.emit(code);
    cb.emit("}");
    cb.emit("");
}

void LLVM_Comp::callFunc(Call *call, string func_name, Var_Type retrunType, vector<Exp *> arg_list)
{
    TableEntry *ent = sym.getTableEntry(func_name);
    int index = 0;
    std::string code = "call " + operationSize(call->type) + " @" + func_name + "(";
    vector<Var_Type> temp = ent->getTypes();
    if (call->type != V_VOID)
    {
        call->var_name = freshVar();
        code += call->var_name + " = ";
    }

    if (!arg_list.empty())
    {
        for (Exp *e : arg_list)
        {
            if (temp[index] == V_INT && e->type == V_BYTE)
            {
                e->var_name = makeTruncZext(e->var_name, "i8", "i32", "zext");
                e->type = V_INT;
            }

            code += operationSize(e->type) + " " + e->var_name + ", ";
            index++;
        }
        code.erase(code.size() - 2);
    }
    code += ")";
    cb.emit(code);
}

void LLVM_Comp::ExpIfExpElseExp(Exp *exp, Exp *e1, Exp *e2, Exp *e3)
{
    exp->var_name = freshVar();
    string type = operationSize(exp->type);
    CreateBranch(e2);
    AddLabelAfterExpression(e2);
    cb.bpatch(e2->truelist, e2->label);
    string to_emit = exp->var_name + " = " + type + " " + e1->var_name;
    cb.emit(to_emit);
    int location = cb.emit("br label @");
    string false_label = cb.genLabel();
    cb.bpatch(e2->falselist, false_label);
    to_emit = exp->var_name + " = " + type + " " + e3->var_name;
    cb.emit(to_emit);
    string next_label = cb.genLabel();
    cb.bpatch(cb.makelist({location, FIRST}), next_label);
}

void LLVM_Comp::printCodeBuffer()
{
    cb.printGlobalBuffer();
    cb.printCodeBuffer();
}