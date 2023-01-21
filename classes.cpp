#include "llvm_comp.hpp"
/****************************************   TYPE   ****************************************/

Type::Type(Type *t)
{
    this->type = t->type;
    this->value = value;
}

Type::Type(Var_Type v_type)
{
    switch (v_type)
    {
    case (V_INT):
    {
        this->value = "int";
        break;
    }
    case (V_BYTE):
    {
        this->value = "byte";
        break;
    }

    case (V_BOOL):
    {
        this->value = "bool";
        break;
    }

    case (V_VOID):
    {
        this->value = "void";
        break;
    }
    }
    this->type = v_type;
}

/****************************************   STATEMENT   ****************************************/

// Type ID;
Statement::Statement(Type *t, Id *symbol)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
    if (comp.sym.isExist(symbol->value))
    {
        errorDef(yylineno, symbol->value);
    }
    symbol->type = t->type;
    symbol->var_name = comp.freshVar() + "_For_" + symbol->value;
    string emptyVal = "";
    comp.sym.addSymbol(symbol, symbol->var_name);

    TableEntry *ent = comp.sym.getTableEntry(symbol->value);

    string to_emit = symbol->var_name + " = getelementptr i32, i32* " + comp.get_stack_for_function() + ", i32 " + to_string(ent->getOffset());
    comp.emit(to_emit);
    string to_emit2 = "store i32 0, i32* " + symbol->var_name;
    comp.emit(to_emit2);
}

// Type ID = EXP;
Statement::Statement(Type *t, Id *symbol, Exp *exp)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
    comp.cb.bpatch(comp.cb.makelist({exp->location_for_exp, FIRST}), exp->label_for_exp);

    if (comp.sym.isExist(symbol->value))
    {
        errorDef(yylineno, symbol->value);
    }
    if (t->type != exp->type && !(t->type == V_INT && exp->type == V_BYTE))
    {
        errorMismatch(yylineno);
    }

    symbol->type = t->type;
    string to_emit_get = "";
    string to_emit_store = "";
    if (t->type == V_BOOL && !comp.isBoolLiteral(exp->value))
    {
        exp->var_name = comp.DeclareBool(exp);
        exp->var_name = comp.makeTruncZext(exp->var_name, "i1", "i32", "zext");
        symbol->var_name = comp.freshVar() + "_For_" + symbol->value;
        comp.sym.addSymbol(symbol, symbol->var_name);
        TableEntry *ent = comp.sym.getTableEntry(symbol->value);
        to_emit_get = symbol->var_name + " = getelementptr i32, i32* " + comp.get_stack_for_function() + ", i32 " + to_string(ent->getOffset());
        to_emit_store = "store i32 " + exp->var_name + ", i32* " + symbol->var_name;
    }
    else
    {
        if (t->type != V_INT)
        {
            exp->var_name = comp.makeTruncZext(exp->var_name, comp.operationSize(symbol->type), "i32", "zext");
        }
        else if (t->type == V_INT && exp->type == V_BYTE)
        {
            exp->var_name = comp.makeTruncZext(exp->var_name, comp.operationSize(exp->type), "i32", "zext");
        }
        symbol->var_name = comp.freshVar() + "_For_" + symbol->value;
        comp.sym.addSymbol(symbol, symbol->var_name, exp->value);
        TableEntry *ent = comp.sym.getTableEntry(symbol->value);
        to_emit_get = symbol->var_name + " = getelementptr i32, i32* " + comp.get_stack_for_function() + ", i32 " + to_string(ent->getOffset());
        to_emit_store = "store i32 " + exp->var_name + ", i32* " + symbol->var_name;
    }

    comp.emit(to_emit_get);
    comp.emit(to_emit_store);
}

// ID = Exp;
Statement::Statement(Id *symbol, Exp *exp)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
    comp.cb.bpatch(comp.cb.makelist({exp->location_for_exp, FIRST}), exp->label_for_exp);

    TableEntry *ent = comp.sym.getTableEntry(symbol->value);
    if (ent == nullptr || ent->getIsFunc())
    {
        errorUndef(yylineno, symbol->value);
    }

    if (ent->getTypes()[0] != exp->type && !(ent->getTypes()[0] == V_INT && exp->type == V_BYTE))
    {
        errorMismatch(yylineno);
    }

    if (exp->type == V_BOOL && !comp.isBoolLiteral(exp->value))
    {
        exp->var_name = comp.DeclareBool(exp);
    }

    if (exp->type != V_INT)
    {
        exp->var_name = comp.makeTruncZext(exp->var_name, comp.operationSize(exp->type), "i32", "zext");
    }

    string var_name_in_table = ent->getVarName();
    string to_emit = "store i32 " + exp->var_name + ", i32* " + var_name_in_table;
    comp.emit(to_emit);
}

// return Exp;
Statement::Statement(Node *symbol, Exp *exp)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
    comp.cb.bpatch(comp.cb.makelist({exp->location_for_exp, FIRST}), exp->label_for_exp);

    if (symbol->value.compare("return") == 0)
    {
        TableEntry *ent = comp.sym.getTableEntry(comp.sym.currentFunction);
        string type = comp.operationSize(exp->type);
        if (ent->getReturnValue() == V_VOID || !comp.sym.checkReturnType(exp->type))
        {
            errorMismatch(yylineno);
        }
        if (exp->type == V_BOOL && !comp.isBoolLiteral(exp->value))
        {
            exp->var_name = comp.DeclareBool(exp);
        }
        if (exp->type == V_BYTE && ent->getReturnValue() == V_INT)
        {
            exp->var_name = comp.makeTruncZext(exp->var_name, "i8", "i32", "zext");
            type = "i32";
        }

        string to_emit = "ret " + type + " " + exp->var_name;
        comp.emit(to_emit);
    }
}

//
Statement::Statement(Call *call)
{
    this->value = call->value;
    this->type = call->type;
}

// RETURN/ BREAK / CONTINUE
Statement::Statement(Node *symbol)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
    if (symbol->value.compare("return") == 0)
    {
        if (!comp.sym.checkReturnType(V_VOID))
        {
            errorMismatch(yylineno);
        }
        string to_emit = "ret void";
        comp.emit(to_emit);
    }
    else if (symbol->value.compare("break") == 0)
    {
        if (comp.sym.while_labels.empty())
        {
            errorUnexpectedBreak(yylineno);
        }
        string to_emit = "br label @";
        int location = comp.emit(to_emit);
        this->nextlist = comp.cb.makelist({location, FIRST});
    }
    else if (symbol->value.compare("continue") == 0)
    {
        if (comp.sym.while_labels.empty())
        {
            errorUnexpectedContinue(yylineno);
        }
        string to_emit = "br label %" + comp.sym.while_labels.top();
        comp.emit(to_emit);
    }

    this->type = UNDEFINED;
    this->value = symbol->value;
}

/****************************************   CALL   ****************************************/

Call::Call(Id *symbol)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
    TableEntry *ent = comp.sym.getTableEntry(symbol->value);

    if (ent == nullptr || !ent->getIsFunc())
    {
        errorUndefFunc(yylineno, symbol->value);
    }

    if (!ent->getTypes().empty())
    {
        vector temp = convertToStringVector(ent->getTypes());
        errorPrototypeMismatch(yylineno, symbol->value, temp);
    }

    this->value = symbol->value;
    this->type = ent->getReturnValue();
    comp.callFunc(this, symbol->value, this->type, vector<Exp *>());
}

Call::Call(Id *symbol, Explist *exp_list)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
    TableEntry *ent = comp.sym.getTableEntry(symbol->value);
    if (ent == nullptr || !ent->getIsFunc())
    {
        errorUndefFunc(yylineno, symbol->value);
    }

    if (ent->getTypes().size() != exp_list->getExpressions().size())
    {
        vector temp = convertToStringVector(ent->getTypes());
        errorPrototypeMismatch(yylineno, symbol->value, temp);
    }

    int index = 0;
    vector<Exp *> temp = exp_list->getExpressions();
    for (Var_Type t : ent->getTypes())
    {
        if (t != temp[index]->type && !(t == V_INT && temp[index]->type == V_BYTE))
        {
            vector<string> temp = convertToStringVector(ent->getTypes());
            errorPrototypeMismatch(yylineno, symbol->value, temp);
        }
        index++;
    }

    this->value = symbol->value;
    this->type = ent->getReturnValue();
    comp.callFunc(this, symbol->value, this->type, exp_list->getExpressions());
}

/****************************************   EXP_LIST   ****************************************/
Explist::Explist(Exp *exp)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
    if (exp->type == V_BOOL && !(comp.isBoolLiteral(exp->value)))
    {
        comp.DecalreBoolArgFunc(exp);
    }
    this->exp_list.insert(this->exp_list.begin(), exp);
}

Explist::Explist(Exp *exp, Explist *exp_list)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
    this->exp_list = vector<Exp *>(exp_list->exp_list);
    if (exp->type == V_BOOL && !(comp.isBoolLiteral(exp->value)))
    {
        comp.DecalreBoolArgFunc(exp);
    }
    this->exp_list.insert(this->exp_list.begin(), exp);
}

/****************************************   EXP   ****************************************/

Exp::Exp(Exp *exp)
{
    this->value = exp->value;
    this->var_name = exp->var_name;
    this->type = exp->type;
    this->truelist = exp->truelist;
    this->falselist = exp->falselist;
    this->nextlist = exp->nextlist;
    this->label = exp->label;
    this->location_for_exp = exp->location_for_exp;
    this->label_for_exp = exp->label_for_exp;
    this->actul_label_exp = exp->actul_label_exp;
    this->actual_location_exp = exp->actual_location_exp;
}

// Exp IF EXP else EXP
Exp::Exp(Exp *e1, Exp *e2, Exp *e3)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
    if (e2->type != V_BOOL || (e1->type != e3->type && !comp.sym.isValidTypesOperation(e1->type, e3->type)))
    {
        errorMismatch(yylineno);
    }
    if (e1->type == e3->type)
    {
        this->type = e1->type;
    }
    else if (comp.sym.isValidTypesOperation(e1->type, e3->type))
    {
        this->type = V_INT;
        if (e1->type == V_BYTE)
        {
            e1->var_name = comp.makeTruncZext(e1->var_name, "i8", "i32", "zext");
        }
        if (e3->type == V_BYTE)
        {
            e3->var_name = comp.makeTruncZext(e3->var_name, "i8", "i32", "zext");
        }
    }
    comp.ExpIfExpElseExp(this, e1, e2, e3);
}

// EXP BINOP EXP
Exp::Exp(Exp *e1, Node *n, Exp *e2)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
    string exp_label = "br label @";
    this->location_for_exp = comp.emit(exp_label);
    this->label_for_exp = comp.cb.genLabel();
    this->actul_label_exp = e1->actul_label_exp;
    this->actual_location_exp = e1->location_for_exp;

    if (!comp.sym.isValidTypesOperation(e1->type, e2->type))
    {
        errorMismatch(yylineno);
    }

    if (e1->type == V_INT || e2->type == V_INT)
        this->type = V_INT;
    else
        this->type = V_BYTE;

    comp.BinopExp(this, e1, e2, n->value);
}
Exp::Exp (Call* c)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
    string exp_label = "br label @";
    this->location_for_exp = comp.emit(exp_label);
    this->label_for_exp = comp.cb.genLabel();
    this->actul_label_exp = actul_label_exp;
    this->actual_location_exp = location_for_exp;
    this-> value = c->value;
    this-> type = c->type;
    this->var_name = c->var_name;
    this->truelist = c->truelist;
    this->falselist = c->falselist;
    this->nextlist = c->nextlist;
    this->label = c->label;
}

// EXP AND/OR/RELOP EXP
Exp::Exp(Var_Type type, Exp *e1, Node *n1, Exp *e2)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
    string exp_label = "br label @";
    this->location_for_exp = comp.emit(exp_label);
    this->label_for_exp = comp.cb.genLabel();
    this->actul_label_exp = e1->actul_label_exp;
    this->actual_location_exp = e1->location_for_exp;

    this->type = V_BOOL;
    if (e1->type == V_BOOL && e2->type == V_BOOL)
    {
        if (n1->value.compare("and") != 0 && n1->value.compare("or") != 0)
        {
            errorMismatch(yylineno);
        }

        if (n1->value.compare("and") == 0)
        {
            comp.AndExp(this, e1, e2);
        }
        else
        {
            comp.OrExp(this, e1, e2);
        }
    }
    else if (comp.sym.isValidTypesOperation(e1->type, e2->type))
    { // RELOP
        comp.RelopExp(this, e1, e2, n1->value);
    }
    else
    {
        errorMismatch(yylineno);
    }
}

// NOT EXP
Exp::Exp(Node *n, Exp *e)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
    comp.cb.bpatch(comp.cb.makelist({e->location_for_exp, FIRST}), e->label_for_exp);

    string exp_label = "br label @";
    this->location_for_exp = comp.emit(exp_label);
    this->label_for_exp = comp.cb.genLabel();
    this->actul_label_exp = label_for_exp;
    this->actual_location_exp = location_for_exp;

    if (e->type != V_BOOL)
        errorMismatch(yylineno);

    this->type = V_BOOL;
    this->truelist = e->falselist;
    this->falselist = e->truelist;
    this->label = e->label;
    this->var_name = e->var_name;
    this->nextlist = e->nextlist;
    this->value = e->value;

    if (comp.isBoolLiteral(e->value))
    {
        this->value = e->value == "true" ? "false" : "true";
        this->var_name = comp.freshVar();
        string to_emit = this->var_name + " = add i1 0, " + ((this->value == "true") ? "1" : "0");
        comp.emit(to_emit);
    }
}

// (TYPE) EXP
Exp::Exp(Type *t, Exp *e)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
    comp.cb.bpatch(comp.cb.makelist({e->location_for_exp, FIRST}), e->label_for_exp);

    string exp_label = "br label @";
    this->location_for_exp = comp.emit(exp_label);
    this->label_for_exp = comp.cb.genLabel();
    this->actul_label_exp = label_for_exp;
    this->actual_location_exp = location_for_exp;

    if (t->type != e->type)
    {
        if (!comp.sym.isValidTypesOperation(t->type, e->type))
            errorMismatch(yylineno);
    }

    this->type = t->type;
    this->value = e->value;
    this->truelist = e->truelist;
    this->falselist = e->falselist;
    this->nextlist = e->nextlist;

    string v_name = e->var_name;
    if (t->type == V_INT && e->type == V_BYTE)
    {
        this->var_name = comp.makeTruncZext(e->var_name, comp.operationSize(e->type), comp.operationSize(t->type), "zext");
    }
    else if (t->type == V_BYTE && e->type == V_INT)
    {
        this->var_name = comp.makeTruncZext(e->var_name, comp.operationSize(e->type), comp.operationSize(t->type), "trunc");
    }
    else
    {
        this->var_name = e->var_name;
    }
}

// ID
Exp::Exp(Id *id)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
    TableEntry *ent = comp.sym.getTableEntry(id->value);

    string exp_label = "br label @";
    this->location_for_exp = comp.emit(exp_label);
    this->label_for_exp = comp.cb.genLabel();
    this->actul_label_exp = label_for_exp;
    this->actual_location_exp = location_for_exp;
    if (ent == nullptr || ent->getIsFunc())
    {
        errorUndef(yylineno, id->value);
    }
    this->value = ent->getValue();
    this->type = ent->getTypes()[0];
    int offset = ent->getOffset();
    if (offset < 0)
    {
        this->var_name = "%" + to_string(-offset - 1);
    }
    else
    {
        this->var_name = comp.freshVar();
        std::string code = this->var_name + " = load i32 , i32* " + ent->getVarName();
        comp.emit(code);
        if (comp.operationSize(this->type) != "i32")
        {
            this->var_name = comp.makeTruncZext(this->var_name, "i32", comp.operationSize(this->type), "trunc");
        }
    }

    if (this->type == V_BOOL && !comp.isBoolLiteral(this->value))
    {
        comp.CreateBranch(this);
        comp.AddLabelAfterExpression(this);
    }
}

// TRUE/FALSE/NUM/STRING
Exp::Exp(Node *n)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
    string emit_exp = "br label @";
    this->location_for_exp = comp.emit(emit_exp);
    this->label_for_exp = comp.cb.genLabel();
    this->actul_label_exp = label_for_exp;
    this->actual_location_exp = location_for_exp;

    this->value = n->value;
    if (comp.isBoolLiteral(n->value))
    {
        this->type = V_BOOL;
        this->value = n->value;
        this->var_name = comp.freshVar();
        string val = (n->value.compare("true") == 0) ? "1" : "0";
        string to_emit = this->var_name + "= add i1 0," + val;
        comp.emit(to_emit);
    }
    else if (n->type == V_INT)
    {
        this->value = n->value;
        this->type = V_INT;
        this->var_name = comp.freshVar();
        comp.emit(this->var_name + " = add i32 0, " + n->value);
    }
    else if (n->type == V_STRING)
    {
        this->value = n->value;
        this->type = V_STRING;
        string global_name = comp.globalFreshVar();
        n->value.erase(0, 1);
        string str = n->value.erase(n->value.size() - 1);
        string to_emit = global_name + " = internal constant [" + to_string(str.size() + 1) + " x i8] c\"" + n->value + "\\00\"";
        comp.emitGlobal(to_emit);

        this->var_name = comp.freshVar();
        to_emit = this->var_name + " = getelementptr [" + to_string(str.size() + 1) + " x i8], [" + to_string(str.size() + 1) + " x i8]* " + global_name + ", i32 0, i32 0";
        comp.emit(to_emit);
    }
}

// NUM B
Exp::Exp(Node *n1, Node *n2)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
    string emit_exp = "br label @";
    this->location_for_exp = comp.emit(emit_exp);
    this->label_for_exp = comp.cb.genLabel();
    this->actul_label_exp = label_for_exp;
    this->actual_location_exp = location_for_exp;

    if (n1->type != V_INT || n2->value.compare("b") != 0)
        errorMismatch(yylineno);

    this->type = V_BYTE;
    if (stoi(n1->value) > 255)
        errorByteTooLarge(yylineno, n1->value);

    this->value = n1->value;
    this->var_name = comp.freshVar();
    string code = this->var_name + "= add i8 0, " + n1->value;
    comp.emit(code);
}

/****************************************   FORMAL_DECLERATION   ****************************************/
FormalDecl::FormalDecl(Type *type, Id *symbol)
{
    this->type = type->type;
    this->value = symbol->value;
}

/****************************************   FORMALS_LIST   ****************************************/

FormalsList::FormalsList(FormalDecl *f_dec)
{
    this->type = UNDEFINED;
    this->value = "This is a formals list";
    this->declaration.emplace_back(f_dec);
}

FormalsList::FormalsList(FormalDecl *f_dec, FormalsList *f_list)
{
    this->type = f_list->type;
    this->value = f_list->value;
    this->declaration = f_list->declaration;
    this->declaration.insert(declaration.begin(), f_dec);
}

/****************************************   FORMALS  ****************************************/

Formals::Formals(FormalsList *f_list)
{
    this->value = "this is Formals";
    this->declaration = f_list->declaration;
    this->type = UNDEFINED;
}

/**/
