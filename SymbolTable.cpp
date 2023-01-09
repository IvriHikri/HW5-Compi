#include "symbolTable.hpp"

string convertToString(Var_Type t)
{
    string s;
    switch (t)
    {
    case V_INT:
        s = "INT";
        break;
    case V_BYTE:
        s = "BYTE";
        break;
    case V_BOOL:
        s = "BOOL";
        break;
    case V_STRING:
        s = "STRING";
        break;
    case V_VOID:
        s = "VOID";
        break;
    }
    return s;
}

vector<string> convertToStringVector(vector<Var_Type> vec)
{
    vector<string> new_vec = vector<string>();
    for (Var_Type t : vec)
    {

        new_vec.push_back(convertToString(t));
    }

    return new_vec;
}

void openScope()
{
    if (symbolTables.empty())
    {
        symbolTables.emplace_back(new Table());
        string print = "print";
        string printInt = "printi";
        vector<Var_Type> vec_print = vector<Var_Type>(1, V_STRING);
        vector<Var_Type> vec_printInt = vector<Var_Type>(1, V_INT);
        symbolTables.back()->getEntries().emplace_back(new TableEntry(print, vec_print, V_VOID, 0));
        symbolTables.back()->getEntries().emplace_back(new TableEntry(printInt, vec_printInt, V_VOID, 0));
        offset.push(0);
    }
    else
    {
        symbolTables.emplace_back(new Table());
        offset.push(offset.top());
    }
}

void findMain()
{
    TableEntry *ent = getTableEntry(string("main"));
    if (ent == nullptr || ent->getTypes().size() != 0 || ent->getReturnValue() != V_VOID)
    {
        errorMainMissing();
    }
}

void closeScope()
{
    endScope();
    Table *t = symbolTables.back();
    string s;
    for (TableEntry *ent : t->getEntries())
    {
        if (ent->getIsFunc())
        {
            s = convertToString(ent->getReturnValue());
            vector<string> temp = convertToStringVector(ent->getTypes());
            printID(ent->getName(), ent->getOffset(), makeFunctionType(s, temp));
        }
        else
        {
            s = convertToString(ent->getTypes()[0]);
            printID(ent->getName(), ent->getOffset(), s);
        }
    }
    symbolTables.pop_back();
    offset.pop();
}

void closeGlobalScope()
{
    findMain();
    closeScope();
}

void addSymbol(Node *symbol, string &value)
{
    if (isExist(symbol->value))
    {
        errorDef(yylineno, symbol->value);
    }

    symbolTables.back()->getEntries().emplace_back(new TableEntry(symbol->value, symbol->type, offset.top()));
    offset.top()++;
}

void declareFunction(Type *type, Id *id, Formals *formals)
{
    if (symbolTables.empty())
    {
        openScope();
    }

    if (isExist(id->value)) // check if Function identifier already exist
    {
        errorDef(yylineno, id->value);
    }

    vector<Var_Type> var_types;
    for (FormalDecl *f : formals->declaration) // create types vector from function parameters
    {
        var_types.push_back(f->type);
    }

    symbolTables.back()->getEntries().emplace_back(new TableEntry(id->value, var_types, type->type, 0));

    openScope();
    int i = -1;
    for (FormalDecl *f : formals->declaration)
    {
        if (isExist(f->value))
        {
            errorDef(yylineno, f->value);
        }
        symbolTables.back()->getEntries().emplace_back(new TableEntry(f->value, f->type, i));
        i--;
    }
    currentFunction = id->value;
}

bool isExist(string id)
{
    for (Table *t : symbolTables)
    {
        for (TableEntry *ent : t->getEntries())
        {
            if (ent->getName().compare(id) == 0)
            {
                return true;
            }
        }
    }
    return false;
}

TableEntry *getTableEntry(string id)
{
    for (Table *t : symbolTables)
    {
        for (TableEntry *ent : t->getEntries())
        {
            if (ent->getName().compare(id) == 0)
            {
                return ent;
            }
        }
    }
    return nullptr;
}

bool checkReturnType(Var_Type type)
{
    TableEntry *ent = getTableEntry(currentFunction);
    if (ent == nullptr || !ent->getIsFunc())
    {
        // will be if we closed the scope of a function and then "currentFunction" = ""...
        //  shouldn't happen
        exit(1);
    }

    return ((ent->getReturnValue() == type) || (ent->getReturnValue() == V_INT && type == V_BYTE));
}

void checkExpBool(Exp *exp)
{
    if (exp->type != V_BOOL)
    {
        errorMismatch(yylineno);
    }
}

void setCurrFunction(string newFunc)
{
    currentFunction = newFunc;
}

void start_while()
{
    in_while++;
}
void finish_while()
{
    in_while--;
}

bool isValidTypesOperation(Var_Type type1, Var_Type type2)
{
    if (!(type1 == V_INT || type1 == V_BYTE) && !(type2 == V_INT || type2 == V_BYTE))
    {
        return false;
    }

    return true;
}