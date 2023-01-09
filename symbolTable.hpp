#ifndef SYMBOL_TABLE
#define SYMBOL_TABLE

#include <vector>
#include <string>
#include "classes.h"

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
bool isValidTypesOperation(Var_Type type1, Var_Type type2);

/// Semantic Utilities ///

static list<Table *> symbolTables = list<Table *>();
static stack<int> offset = stack<int>();
static int in_while = 0;
static string currentFunction = "";

#endif /*SYMBOL_TABLE*/