#ifndef HW3_SYMTABLE_H
#define HW3_SYMTABLE_H

#include "attributes.h"
#include "hw3_output.hpp"

class SymTableEntry {
public:
    int scope;
    string name;
    int offset;
    Type type;

    // for functions
    vector<Type> arg_types;
    bool is_func;
    bool is_override;
    int func_idx;

    SymTableEntry* next;

    SymTableEntry(int scope,
                  const char* name,
                  int offset,
                  Type type,
                  vector<Type> arg_types,
                  bool is_func,
                  bool is_override,
                  int func_idx,
                  SymTableEntry* next);
    ~SymTableEntry() = default;
};

// linked list of SymTableEntry
class SymTable {
    SymTableEntry* head;
    int curr_scope;
    vector<int> offset_stack;

public:
    SymTable();
    ~SymTable();

    void addVarSymbol(const char* name, Type type);
    void addArgSymbol(const char* name, Type type, int offset);
    void addFuncSymbol(const char* name, Type ret_type, vector<Type> arg_types, bool is_override, int func_idx);

    SymTableEntry* getVarSymbol(const char* name);
    vector<SymTableEntry*> getFuncSymbol(const char* name, vector<Type> arg_types);
    vector<SymTableEntry*> getFuncsByName(const char* func_name);

    void deleteTopScope();
    void addNewScope();
    void printTopScope();
};


#endif //HW3_SYMTABLE_H
