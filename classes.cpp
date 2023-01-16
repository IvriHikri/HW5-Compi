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
    symbol->var_name = comp.freshVar() + "For_" + symbol->value;
    string emptyVal = "";
    comp.sym.addSymbol(symbol, symbol->var_name);

    TableEntry *ent = comp.sym.getTableEntry(symbol->value);

    string to_emit = symbol->var_name + " = getelementptr i32, i32* " + comp.get_stack_for_function() + ", i32 0, i32" + to_string(ent->getOffset());
    comp.emit(to_emit);
    string to_emit2 = "store i32 0, i32* " + symbol->var_name;
    comp.emit(to_emit2);
}

// Type ID = EXP;
Statement::Statement(Type *t, Id *symbol, Exp *exp)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
    if (comp.sym.isExist(symbol->value))
    {
        errorDef(yylineno, symbol->value);
    }
    if (t->type != exp->type && !(t->type == V_INT && exp->type == V_BYTE))
    {
        errorMismatch(yylineno);
    }
    symbol->type = t->type;
    if (t->type == V_BOOL && !comp.isBoolLiteral(exp->value))
    {
        symbol->var_name = comp.DeclareBool(exp);
        // TODO, need to add some things...
    }
    else if (t->type != V_INT)
    {
        symbol->var_name = comp.makeTruncZext(symbol->var_name, comp.operationSize(symbol->type), "i32", "zext");
    }
    symbol->var_name = symbol->var_name + "_For_" + symbol->value;
    comp.sym.addSymbol(symbol, symbol->var_name);

    TableEntry *ent = comp.sym.getTableEntry(symbol->value);
    string to_emit = symbol->var_name + " = getelementptr i32, i32* " + comp.get_stack_for_function() + ", i32 0, i32 " + to_string(ent->getOffset());
    comp.emit(to_emit);
    to_emit = "store i32 " + exp->var_name + ", i32* " + symbol->var_name;
    comp.emit(to_emit);
}

// ID = Exp;
Statement::Statement(Id *symbol, Exp *exp)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
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

    else if (exp->type != V_INT)
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
    if (symbol->value.compare("return") == 0)
    {
        TableEntry *ent = comp.sym.getTableEntry(comp.sym.currentFunction);
        if (ent->getReturnValue() == V_VOID || !comp.sym.checkReturnType(exp->type))
        {
            errorMismatch(yylineno);
        }
        string type = comp.operationSize(exp->type);
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
    this->exp_list.insert(this->exp_list.begin(), exp);
}

Explist::Explist(Exp *exp, Explist *exp_list)
{
    this->exp_list = vector<Exp *>(exp_list->exp_list);
    this->exp_list.insert(this->exp_list.begin(), exp);
}

/****************************************   EXP   ****************************************/

// (Exp)
/*Exp::Exp(Exp *exp)
{
    this->value = exp->value;
    this->type = exp->type;
    this->var_name = exp->var_name;
    this->falselist = exp->falselist;
    this->truelist = exp->truelist;
    this->nextlist = exp->nextlist;
}*/

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
    if (!comp.sym.isValidTypesOperation(e1->type, e2->type))
    {
        errorMismatch(yylineno);
    }

    if (e1->type == V_INT || e2->type == V_INT)
        this->type = V_INT;
    else
        this->type = V_BYTE;

    string var_name1 = e1->var_name;
    string var_name2 = e2->var_name;

    if (this->type == V_INT)
    {
        if (e1->type == V_BYTE)
        {
            var_name1 = comp.makeTruncZext(var_name1, "i8", "i32", "zext");
        }
        if (e2->type == V_BYTE)
        {
            var_name2 = comp.makeTruncZext(var_name2, "i8", "i32", "zext");
        }
    }

    this->value = e1->value + " " + n->value + " " + e2->value;
    this->var_name = comp.freshVar();
    string op = comp.whichOP(n->value, this->type);
    if((op.compare("udiv") == 0 || op.compare("sdiv") == 0) && e2->value.compare("0") == 0 )
    {
        //Need to add what we need to do here...
    }
    string to_emit = this->var_name + "= " + op + " " + comp.operationSize(this->type) + " " + var_name1 + ", " + var_name2;
    comp.emit(to_emit);

    /*NEED TO CHECK DIV BY ZERO*/
}

// EXP AND/OR/RELOP EXP
Exp::Exp(Var_Type type, Exp *e1, Node *n1, Exp *e2)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
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
    if (e->type != V_BOOL)
        errorMismatch(yylineno);

    this->type = V_BOOL;
    if (comp.isBoolLiteral(n->value))
    {
        this->value = n->value == "true" ? "false" : "true";
    }

    this->truelist = e->falselist;
    this->falselist = e->truelist;
}

// (TYPE) EXP
Exp::Exp(Type *t, Exp *e)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
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
        this->var_name = comp.makeTruncZext(e->var_name, comp.operationSize(e->type), comp.operationSize(t->type), "trunc");
    }
    else if (t->type == V_BYTE && e->type == V_INT)
    {
        this->var_name = comp.makeTruncZext(e->var_name, comp.operationSize(e->type), comp.operationSize(t->type), "zext");
    }
    else
    {
        this->var_name = e->var_name;
    }
}

// Call
/*Exp::Exp(Call *c)
{
    this->type = c->type;
    this->value = c->value;
}*/

// ID
Exp::Exp(Id *id)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
    TableEntry *ent = comp.sym.getTableEntry(id->value);
    if (ent == nullptr || ent->getIsFunc())
    {
        errorUndef(yylineno, id->value);
    }
    this->value = ent->getName();
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
}

// TRUE/FALSE/NUM/STRING
Exp::Exp(Node *n)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
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
        string str = n->value.erase(n->value.size() - 1); // delete end "
        string to_emit = global_name + "= internal constant [" + to_string(str.size() + 1) + " x i8] c" + n->value + "\00\"";
        comp.emitGlobal(to_emit);

        this->var_name = comp.freshVar();
        to_emit = this->var_name + " getelementptr [" + to_string(str.size() + 1) + " x i8], [" + to_string(str.size() + 1) + " x i8]* " + global_name + ", i32 0, i32 0";
        comp.emit(to_emit);
    }
}

// NUM B
Exp::Exp(Node *n1, Node *n2)
{
    LLVM_Comp &comp = LLVM_Comp::getInstance();
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
