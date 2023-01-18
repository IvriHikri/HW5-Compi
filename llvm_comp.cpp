#include "llvm_comp.hpp"
#include "hw3_output.hpp"
#include "bp.hpp"
#include "symbolTable.hpp"

int LLVM_Comp::emit(string to_emit)
{
    return cb.emit(to_emit);
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
    if (isBoolLiteral(e2->value))
    {
        CreateBranch(e2);
        AddLabelAfterExpression(e2);
    }

    exp->var_name = e2->var_name;
    exp->truelist = e2->truelist;
    exp->falselist = cb.merge(e1->falselist, e2->falselist);
    exp->label = e2->label;
}

void LLVM_Comp::OrExp(Exp *exp, Exp *e1, Exp *e2)
{
    cb.bpatch(e1->falselist, e1->label);
    if (isBoolLiteral(e2->value))
    {
        CreateBranch(e2);
        AddLabelAfterExpression(e2);
    }
    exp->var_name = e2->var_name;
    exp->falselist = e2->falselist;
    exp->truelist = cb.merge(e1->truelist, e2->truelist);
    exp->label = e2->label;
}

void LLVM_Comp::RelopExp(Exp *exp, Exp *e1, Exp *e2, string rel)
{
    Var_Type type = (e1->type == V_BYTE && e2->type == V_BYTE) ? V_BYTE : V_INT;
    string relop = whichRelop(rel, type);

    string var_name1 = e1->var_name;
    string var_name2 = e2->var_name;

    if (e1->type != type)
    {
        var_name1 = makeTruncZext(var_name1, operationSize(e1->type), operationSize(type), "zext");
    }
    if (e2->type != type)
    {
        var_name2 = makeTruncZext(var_name2, operationSize(e1->type), operationSize(type), "zext");
    }
    exp->var_name = freshVar();
    string to_emit = exp->var_name + "= icmp " + relop + " " + operationSize(type) + " " + var_name1 + ", " + var_name2;
    cb.emit(to_emit);
    CreateBranch(exp);
    AddLabelAfterExpression(exp);
}

void LLVM_Comp::BinopExp(Exp *exp, Exp *e1, Exp *e2, string operation)
{
    string var_name1 = e1->var_name;
    string var_name2 = e2->var_name;

    string op = whichOP(operation, exp->type);
    if ((op.compare("udiv") == 0 || op.compare("sdiv") == 0))
    {
        std::string division_res = freshVar();
        string to_emit = division_res + " = icmp eq " + operationSize(e2->type) + " 0, " + var_name2;
        cb.emit(to_emit);
        to_emit = "br i1 " + division_res + ", label @, label @";
        int loc = cb.emit(to_emit);
        std::string divided_by_zero_label = cb.genLabel();
        cb.bpatch(cb.makelist({loc, FIRST}), divided_by_zero_label);

        std::string divided_by_zero_string = freshVar();
        cb.emit(divided_by_zero_string + " = " + "getelementptr inbounds [23 x i8], [23 x i8]* @.zero_division_str, i32 0, i32 0");
        cb.emit("call void @print(i8* " + divided_by_zero_string + ")");
        cb.emit("call void @exit(i32 0)");
        int after_div = cb.emit("br label @");
        std::string not_divided_by_zero_label = cb.genLabel();

        cb.bpatch(cb.merge(cb.makelist({loc, SECOND}), cb.makelist({after_div, FIRST})), not_divided_by_zero_label);
    }

    if (exp->type == V_INT)
    {
        if (e1->type == V_BYTE)
        {
            var_name1 = makeTruncZext(var_name1, "i8", "i32", "zext");
        }
        if (e2->type == V_BYTE)
        {
            var_name2 = makeTruncZext(var_name2, "i8", "i32", "zext");
        }
    }

    exp->value = e1->value + " " + operation + " " + e2->value;
    exp->var_name = freshVar();
    string to_emit = exp->var_name + "= " + op + " " + operationSize(exp->type) + " " + var_name1 + ", " + var_name2;
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

    stack_for_function = this->freshVar() + "_" + sym.currentFunction;
    code = stack_for_function + " = alloca i32, i32 50";
    cb.emit(code);
}

string LLVM_Comp::DeclareBool(Exp *exp)
{
    string temp_reg = freshVar();
    int location = cb.emit("br label @");
    string true_label = cb.genLabel();
    cb.bpatch(cb.merge(exp->truelist, cb.makelist({location, FIRST})), true_label);

    int location_for_true = emit("br label @");
    string false_label = cb.genLabel();
    cb.bpatch(exp->falselist, false_label);
    int location_for_false = emit("br label @");

    string phi_label = cb.genLabel();
    cb.bpatch(cb.makelist({location_for_true, FIRST}), phi_label);
    cb.bpatch(cb.makelist({location_for_false, FIRST}), phi_label);
    string to_emit = temp_reg + " = phi i1 [ 1, %" + true_label + "], [ 0, %" + false_label + "]";
    emit(to_emit);
    string bool_reg = makeTruncZext(temp_reg, "i1", "i32", "zext");
    return bool_reg;
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
    stack_for_function = "";
}

void LLVM_Comp::callFunc(Call *call, string func_name, Var_Type retrunType, vector<Exp *> arg_list)
{
    TableEntry *ent = sym.getTableEntry(func_name);
    int index = 0;
    std::string code = "";
    if (call->type != V_VOID)
    {
        call->var_name = freshVar();
        code += call->var_name + " = ";
    }
    code += "call " + operationSize(call->type) + " @" + func_name + "(";
    vector<Var_Type> temp = ent->getTypes();

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

void LLVM_Comp::startIF(Exp *exp)
{
    sym.checkExpBool(exp);
    if (isBoolLiteral(exp->value))
    {
        this->CreateBranch(exp);
        this->AddLabelAfterExpression(exp);
    }

    cb.bpatch(exp->truelist, exp->label);
}

void LLVM_Comp::endIF(Exp *exp, Statement *st)
{
    int after_if = cb.emit("br label @");
    string after_statement_label = cb.genLabel();
    cb.bpatch(cb.merge(exp->falselist, cb.makelist({after_if, FIRST})), after_statement_label);
}

void LLVM_Comp::startElse(Node *symbol)
{
    // Need to have unconditional jump after if to avoid else incase we don't need it
    string to_emit = "br label @";
    int location = cb.emit(to_emit);

    // generate label for else and bpatch it with Exp falselist later on
    symbol->label = cb.genLabel();
    symbol->nextlist = cb.makelist({location, FIRST});
}

void LLVM_Comp::endElse(Exp *exp, Statement *s1, Node *else_symbol, Statement *s2)
{
    int after_else = cb.emit("br label @");
    string after_second_statement_label = cb.genLabel();
    cb.bpatch(cb.merge(else_symbol->nextlist, cb.makelist({after_else, FIRST})), after_second_statement_label);
    cb.bpatch(exp->falselist, else_symbol->label);
    s2->nextlist = cb.merge(s1->nextlist, s2->nextlist);
}

void LLVM_Comp::start_while()
{
    int before_while = cb.emit("br label @");
    string whileCondLabel = cb.genLabel();
    cb.bpatch(cb.makelist({before_while, FIRST}), whileCondLabel);
    sym.while_labels.push(whileCondLabel);
}

void LLVM_Comp::end_while(Exp *exp, Statement *st)
{
    string to_emit = "br label %" + sym.while_labels.top();
    int loc = cb.emit(to_emit);
    cb.bpatch(cb.merge(exp->falselist, st->nextlist), cb.genLabel());
    sym.while_labels.pop();
}

void LLVM_Comp::printCodeBuffer()
{
    cb.printGlobalBuffer();
    cb.printCodeBuffer();
}

void LLVM_Comp::mergeLists(Statement *sts, Statement *st)
{
    sts->nextlist = cb.merge(sts->nextlist, st->nextlist);
    sts->truelist = cb.merge(sts->truelist, st->truelist);
    sts->falselist = cb.merge(sts->falselist, st->falselist);
}

void LLVM_Comp::openGlobalScope()
{
    sym.openScope();
    CodeBuffer &cb = CodeBuffer::instance();
    cb.emit("declare i32 @printf(i8*, ...)");
    cb.emit("declare void @exit(i32)");
    cb.emit("@.int_specifier = constant [4 x i8] c\"%d\\0A\\00\"");
    cb.emit("@.str_specifier = constant [4 x i8] c\"%s\\0A\\00\"");
    cb.emit("@.zero_division_str = internal constant [23 x i8] c\"Error division by zero\\00\"");
    string printi_code = "define void @printi(i32) {\n"
                         "%spec_ptr = getelementptr [4 x i8], [4 x i8]* @.int_specifier, i32 0, i32 0\n"
                         "call i32 (i8*, ...) @printf(i8* %spec_ptr, i32 %0)\n"
                         "ret void\n}";

    string print_code = "define void @print(i8*) {\n"
                        "%spec_ptr = getelementptr [4 x i8], [4 x i8]* @.str_specifier, i32 0, i32 0\n"
                        "call i32 (i8*, ...) @printf(i8* %spec_ptr, i8* %0)\n"
                        "ret void\n}";
    cb.emit("");
    cb.emit(printi_code);
    cb.emit("");
    cb.emit(print_code);
    cb.emit("");
}