#include "SymTable.hpp"

bool argTypesCompatible(vector<Type>& expected_types, vector<Type>& arg_types);
void recPrintTopScope(SymTableEntry* curr, int scope);
void printSymbol(SymTableEntry* sym);

SymTableEntry::SymTableEntry(
        int scope,
        const char* name,
        int offset,
        Type type,
        vector<Type> arg_types = {},
        bool is_func = false,
        bool is_override = false,
        int func_idx = 0,
        SymTableEntry* next = nullptr
        )
: scope(scope), name(string(name)), offset(offset),
  type(type), arg_types(move(arg_types)), is_func(is_func),
  is_override(is_override), func_idx(func_idx), next(next)
{}

SymTable::SymTable() {
    head = nullptr;
    curr_scope = 0;
    offset_stack.push_back(0);
}

SymTable::~SymTable() {
    while(head){
        SymTableEntry* temp = head;
        head = head->next;
        delete temp;
    }
}

void SymTable::addVarSymbol(const char* name, Type type) {
    // assuming the caller checked before that var doesn't exists already
    SymTableEntry* new_sym = new SymTableEntry(curr_scope, name, offset_stack.back(), type);
    new_sym->next = head;
    head = new_sym;
    offset_stack.back()++;
}

void SymTable::addArgSymbol(const char* name, Type type, int offset) {
    SymTableEntry* new_sym = new SymTableEntry(curr_scope, name, offset, type);
    new_sym->next = head;
    head = new_sym;
}

void SymTable::addFuncSymbol(const char* name, Type ret_type, vector<Type> arg_types, bool is_override, int func_idx) {
    // assuming the caller checked before that same func doesn't exit already
    // scope and offset should be 0
    head = new SymTableEntry(curr_scope, name, offset_stack.back(), ret_type,
                             move(arg_types), true, is_override, func_idx, head);
}

SymTableEntry* SymTable::getVarSymbol(const char* name) {
    SymTableEntry* curr = head;

    while(curr) {
        if(curr->name == name && !curr->is_func) {
            return curr;
        }
        curr = curr->next;
    }

    return nullptr;
}

bool argTypesCompatible(vector<Type>& expected_types, vector<Type>& arg_types){
    if(expected_types.size() != arg_types.size()) {
        return false;
    }

    for(int i = 0; i < expected_types.size(); i++){
        bool is_compatible = arg_types[i] == expected_types[i] || (expected_types[i] == INT_TYPE && arg_types[i] == BYTE_TYPE);
        if(!is_compatible){
            return false;
        }
    }

    return true;
}

vector<SymTableEntry*> SymTable::getFuncSymbol(const char *name, vector<Type> arg_types) {
    SymTableEntry* curr = head;
    vector<SymTableEntry*> candidates;

    while(curr) {
        if(curr->name == name && curr->is_func
           && argTypesCompatible(curr->arg_types, arg_types)) {
            candidates.push_back(curr);
        }
        curr = curr->next;
    }

    return candidates;
}

vector<SymTableEntry*> SymTable::getFuncsByName(const char *func_name) {
    SymTableEntry* curr = head;
    vector<SymTableEntry*> matches;

    while(curr) {
        if(curr->name == func_name && curr->is_func) {
            matches.push_back(curr);
        }
        curr = curr->next;
    }

    return matches;
}

void SymTable::deleteTopScope() {
    while(head && head->scope == curr_scope){
        SymTableEntry* temp = head;
        head = head->next;
        delete temp;
    }

    curr_scope--;
    offset_stack.pop_back();
}

void SymTable::addNewScope() {
    curr_scope++;
    offset_stack.push_back(offset_stack.back());
}

void printSymbol(SymTableEntry* sym){
    vector<string> type_to_string = {"STRING","INT","BYTE","BOOL","VOID"};

    if(!sym->is_func) {
        output::printID(sym->name, sym->offset, type_to_string[sym->type]);
        return;
    }
    // symbol is a function
    vector<string> str_arg_types;
    for(auto& type: sym->arg_types){
        str_arg_types.push_back(type_to_string[type]);
    }
    output::printID(sym->name, sym->offset, output::makeFunctionType(type_to_string[sym->type], str_arg_types));
}

void recPrintTopScope(SymTableEntry* curr, int scope){
    if(!curr || curr->scope != scope){
        return;
    }
    recPrintTopScope(curr->next, scope);
    printSymbol(curr);
}

void SymTable::printTopScope() {
    output::endScope();
    recPrintTopScope(head, curr_scope);
}