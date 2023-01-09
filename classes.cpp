#include "symbolTable.hpp"

/* Semantic Checkings*/

static LLVM_Comp comp;

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
    if (isExist(symbol->value))
    {
        errorDef(yylineno, symbol->value);
    }
    symbol->type = t->type;
    symbol->var_name = comp.freshVar();
    string emptyVal = "";
    addSymbol(symbol, emptyVal);
}

// Type ID = EXP;
Statement::Statement(Type *t, Id *symbol, Exp *exp)
{
    if (isExist(symbol->value))
    {
        errorDef(yylineno, symbol->value);
    }
    if (t->type != exp->type && !(t->type == V_INT && exp->type == V_BYTE))
    {
        errorMismatch(yylineno);
    }

    symbol->type = t->type;
    symbol->var_name = comp.freshVar();
    addSymbol(symbol, exp->value);
}

// ID = Exp;
Statement::Statement(Id *symbol, Exp *exp)
{
    TableEntry *ent = getTableEntry(symbol->value);
    if (ent == nullptr || ent->getIsFunc())
    {
        errorUndef(yylineno, symbol->value);
    }

    if (ent->getTypes()[0] != exp->type && !(ent->getTypes()[0] == V_INT && exp->type == V_BYTE))
    {
        errorMismatch(yylineno);
    }
    symbol->var_name = comp.freshVar();
}

// return Exp;
Statement::Statement(Node *symbol, Exp *exp)
{
    if (symbol->value.compare("return") == 0)
    {
        TableEntry *ent = getTableEntry(currentFunction);
        if (ent->getReturnValue() == V_VOID || !checkReturnType(exp->type))
        {
            errorMismatch(yylineno);
        }
    }
}

// Call
Statement::Statement(Call *call)
{
    this->value = call->value;
    this->type = call->type;
}

// RETURN/ BREAK / CONTINUE
Statement::Statement(Node *symbol)
{
    if (symbol->value.compare("return") == 0)
    {
        if (!checkReturnType(V_VOID))
        {
            errorMismatch(yylineno);
        }
    }
    else if (symbol->value.compare("break") == 0)
    {
        if (in_while <= 0)
        {
            errorUnexpectedBreak(yylineno);
        }
    }
    else if (symbol->value.compare("continue") == 0)
    {
        if (in_while <= 0)
        {
            errorUnexpectedContinue(yylineno);
        }
    }

    this->type = UNDEFINED;
    this->value = symbol->value;
}

/****************************************   CALL   ****************************************/

Call::Call(Id *symbol)
{
    TableEntry *ent = getTableEntry(symbol->value);

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
}

Call::Call(Id *symbol, Explist *exp_list)
{
    TableEntry *ent = getTableEntry(symbol->value);
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
            vector temp = convertToStringVector(ent->getTypes());
            errorPrototypeMismatch(yylineno, symbol->value, temp);
        }
        index++;
    }

    this->value = symbol->value;
    this->type = ent->getReturnValue();
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
Exp::Exp(Exp *exp)
{
    this->value = exp->value;
    this->type = exp->type;
    this->var_name = exp->var_name;
}

// Exp IF EXP else EXP
Exp::Exp(Exp *e1, Exp *e2, Exp *e3)
{
    if (e2->type != V_BOOL || (e1->type != e3->type && !((e1->type == V_BYTE && e3->type == V_INT) || (e1->type == V_INT && e3->type == V_BYTE))))
    {
        errorMismatch(yylineno);
    }

    if (e1->type == e3->type)
    {
        this->type = e1->type;
    }
    else if ((e1->type == V_BYTE && e3->type == V_INT) || (e1->type == V_INT && e3->type == V_BYTE))
    {
        this->type = V_INT;
    }

    this->value = e1->value + " OR " + e3->value;
}

// EXP BINOP EXP
Exp::Exp(Exp *e1, Node *n, Exp *e2)
{
    if (!isValidTypesOperation(e1->type, e2->type))
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
            var_name1 = comp.makeZext(var_name1, "i8", "i32");
        }
        if (e2->type == V_BYTE)
        {
            var_name2 = comp.makeZext(var_name2, "i8", "i32");
        }
    }

    string op = comp.whichOP(n->value);
    this->value = e1->value + " " + n->value + " " + e2->value;
    this->var_name = comp.freshVar();
    string to_emit = this->var_name + "= " + op + " " + comp.operationSize(this->type) + " " + var_name1 + " , " + var_name2;
    cb.emit(to_emit);
}

// EXP AND/OR/RELOP EXP
Exp::Exp(Var_Type type, Exp *e1, Node *n1, Exp *e2)
{
    this->type = V_BOOL;
    if (e1->type == V_BOOL && e2->type == V_BOOL)
    {
        if (n1->value.compare("and") != 0 && n1->value.compare("or") != 0)
        {
            errorMismatch(yylineno);
        }
        this->value = "EXP AND/OR EXP";
    }
    else if (isValidTypesOperation(e1->type, e2->type))
    {
        this->value = "EXP RELOP EXP";
    }
    else
    {
        errorMismatch(yylineno);
    }
}

// NOT EXP
Exp::Exp(Node *n, Exp *e)
{
    if (e->type != V_BOOL || n->value.compare("not") != 0)
        errorMismatch(yylineno);

    this->type = V_BOOL;
    this->value = "NOT EXP";
}

// (TYPE) EXP
Exp::Exp(Type *t, Exp *e)
{
    if (t->type != e->type)
    {
        if (!(t->type == V_BYTE && e->type == V_INT) && !(t->type == V_INT && e->type == V_BYTE))
            errorMismatch(yylineno);
    }
    this->type = t->type;
    this->value = e->value;
}

// Call
Exp::Exp(Call *c)
{
    this->type = c->type;
    this->value = c->value;
}

// ID
Exp::Exp(Id *id)
{
    TableEntry *ent = getTableEntry(id->value);
    if (ent == nullptr || ent->getIsFunc())
    {
        errorUndef(yylineno, id->value);
    }
    this->value = ent->getName();
    this->type = ent->getTypes()[0];
    this->var_name = id->var_name;
}

// TRUE/FALSE/NUM/STRING
Exp::Exp(Node *n)
{
    this->value = n->value;
    if (n->value.compare("true") == 0 || n->value.compare("false") == 0)
    {
        this->type = V_BOOL;
    }
    else if (n->type == V_INT)
    {
        this->value = n->value;
        this->type = V_INT;
        this->var_name = comp.freshVar();
        cb.emit(this->var_name + ":=" + n->value);
    }
    else if (n->type == V_STRING)
    {
        this->value = n->value;
        this->type = V_STRING;
    }
}

// NUM B
Exp::Exp(Node *n1, Node *n2)
{
    if (n1->type != V_INT || n2->value.compare("b") != 0)
        errorMismatch(yylineno);

    this->type = V_BYTE;
    if (stoi(n1->value) > 255)
        errorByteTooLarge(yylineno, n1->value);

    this->value = n1->value;
    this->var_name = comp.freshVar();
    string code = this->var_name + ":=" + n1->value;
    cb.emit(code);
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
