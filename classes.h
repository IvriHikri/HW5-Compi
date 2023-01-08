#ifndef CLASSES_H
#define CLASSES_H

#include "hw3_output.hpp"
#include "llvm_comp.hpp"
#include <vector>
#include <string>
#include <list>
#include <stack>
#include <iostream>
extern int yylineno;
using namespace output;
using namespace std;

enum Var_Type
{
    V_INT,
    V_VOID,
    V_BOOL,
    V_BYTE,
    V_STRING,
    UNDEFINED
};

class Node
{
public:
    string value;
    Var_Type type;

    Node(){};
    Node(string token_value) : value()
    {
        value = token_value;
        type = UNDEFINED;
    }
    Node(string token_value, Var_Type type) : value(), type()
    {
        this->value = token_value;
        this->type = type;
    }
};

#define YYSTYPE Node *

/*************************************         CLASSES        **********************************************************/
class Exp;
class Type;
class Call;
class FormalDecl;
class Formals;
class FormalsList;

class Id : public Node
{
public:
    string var_name;
    Id(string value,string Var_name) : Node(value),var_name("") {}
};

class Program : public Node
{
};

class Statement : public Node
{
public:
    // Type ID;
    explicit Statement(Type *t, Id *symbol);

    // Type ID = Exp;
    explicit Statement(Type *t, Id *symbol, Exp *exp);

    // ID = Exp;
    explicit Statement(Id *symbol, Exp *exp);

    // Return Exp;
    explicit Statement(Node *symbol, Exp *exp);

    // Call;
    explicit Statement(Call *call);

    // BREAK/ CONTINUE/ RETURN
    explicit Statement(Node *symbol);
};

class Exp;
class Explist;

class Call : public Node
{
public:
    Call(Id *n);
    Call(Id *n, Explist *exp_list);
};

class Explist : public Node
{
    vector<Exp *> exp_list;

public:
    Explist(Exp *exp);
    Explist(Exp *exp, Explist *exp_list);

    vector<Exp *> getExpressions() { return this->exp_list; }
};

class Exp : public Node
{
public:
    string var_name;
    // (Exp)
    Exp(Exp *exp);

    // Exp IF EXP else EXP
    Exp(Exp *e1, Exp *e2, Exp *e3);

    // EXP BINOP EXP
    Exp(Exp *e1, Node *n, Exp *e2);

    // EXP AND/OR/RELOP EXP
    Exp(Var_Type type, Exp *e1, Node *n, Exp *e2);

    // NOT EXP
    Exp(Node *n, Exp *e);

    // (TYPE) EXP
    Exp(Type *t, Exp *e);

    // Call
    Exp(Call *c);

    // TRUE/FALSE/NUM/STRING
    Exp(Node *n);

    // ID
    Exp(Id *id);

    // NUM B
    Exp(Node *n1, Node *n2);
};

class Type : public Node
{
public:
    explicit Type(Type *t);
    explicit Type(Var_Type v_type);
};

class FormalDecl : public Node
{
public:
    FormalDecl(Type *type, Id *symbol);
};

class Formals : public Node
{
public:
    vector<FormalDecl *> declaration;
    Formals() = default;
    Formals(FormalsList *f_list);
};

class FormalsList : public Formals
{
public:
    FormalsList(FormalDecl *f_dec);
    FormalsList(FormalDecl *f_dec, FormalsList *f_list);
};

class TableEntry
{
    string name;
    vector<Var_Type> types;
    Var_Type returnValue;
    int offset;
    bool isFunc;

public:
    TableEntry();
    explicit TableEntry(string name, Var_Type type, int offset) // for single symbol
    {
        this->name = name;
        this->types = vector<Var_Type>(1, type);
        this->returnValue = UNDEFINED;
        this->offset = offset;
        this->isFunc = false;
    }

    explicit TableEntry(string &name, vector<Var_Type> &types, Var_Type returnValue, int offset) // for function
    {
        this->name = name;
        this->types = types;
        this->returnValue = returnValue;
        this->offset = offset;
        this->isFunc = true;
    }

    ~TableEntry() = default;

    string &getName() { return this->name; }
    vector<Var_Type> &getTypes() { return this->types; }
    Var_Type getReturnValue() { return this->returnValue; }
    int getOffset() { return this->offset; }
    bool getIsFunc() { return this->isFunc; }
};

class Table
{
    vector<TableEntry *> symbols;

public:
    Table()
    {
        symbols = vector<TableEntry *>();
    }

    ~Table() = default;

    vector<TableEntry *> &getEntries()
    {
        return this->symbols;
    }
};

void addSymbol(Node *symbol, string &value);
void declareFunction(Type *type, Id *id, Formals *formals);
bool isExist(string id);
TableEntry *getTableEntry(string id);
void openScope();
void closeScope();
void closeGlobalScope();
void findMain();
bool checkReturnType(Var_Type type);
string convertToString(Var_Type t);
vector<string> convertToStringVector(vector<Var_Type> vec);
void start_while();
void finish_while();
void checkExpBool(Exp *exp);
void setCurrFunction(string newFunc = "");
#endif /*CLASSES_H*/