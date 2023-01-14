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
    string var_name;

public:
    TableEntry();
    explicit TableEntry(string name, Var_Type type, int offset, string var_name) // for single symbol
    {
        this->name = name;
        this->types = vector<Var_Type>(1, type);
        this->returnValue = UNDEFINED;
        this->offset = offset;
        this->isFunc = false;
        this->var_name = var_name;
    }

    explicit TableEntry(string &name, vector<Var_Type> &types, Var_Type returnValue, int offset) // for function
    {
        this->name = name;
        this->types = types;
        this->returnValue = returnValue;
        this->offset = offset;
        this->isFunc = true;
        this->var_name = "";
    }

    ~TableEntry() = default;

    string &getName() { return this->name; }
    vector<Var_Type> &getTypes() { return this->types; }
    Var_Type getReturnValue() { return this->returnValue; }
    int getOffset() { return this->offset; }
    string getVarName() { return var_name; }
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

class SymbolTable{
    public:
    list<Table *> symbolTables;
    stack<int> offset;
    int in_while;
    string currentFunction;

    SymbolTable();
    static SymbolTable &instance();
    void addSymbol(Id *symbol, string var_name);
    void declareFunction(Var_Type type, Id* id, Formals *formals);
    bool isExist(string id);
    TableEntry *getTableEntry(string id);
    void openScope();
    void closeScope();
    void closeGlobalScope();
    void findMain();
    bool checkReturnType(Var_Type type);
    void start_while();
    void finish_while();
    void checkExpBool(Exp *exp);
    void setCurrFunction(string newFunc = "");
    bool isValidTypesOperation(Var_Type type1, Var_Type type2);
};


string convertToString(Var_Type t);
vector<string> convertToStringVector(vector<Var_Type> vec);

#endif /*SYMBOL_TABLE*/