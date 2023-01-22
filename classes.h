#ifndef CLASSES_H
#define CLASSES_H

#include "hw3_output.hpp"
#include "bp.hpp"
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
    string var_name;
    string label;
    string name;

    // For Trinary
    int location_for_exp;
    string label_for_exp;
    string actul_label_exp;
    int actual_location_exp;
    bool is_trinary;
    bool is_relop;
    int location_for_literal_in_trinary;
    string label_for_literal_in_trinary;

    vector<pair<int, BranchLabelIndex>> truelist;
    vector<pair<int, BranchLabelIndex>> falselist;
    vector<pair<int, BranchLabelIndex>> nextlist;

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
    Id(string value) : Node(value) {}
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

#endif /*CLASSES_H*/