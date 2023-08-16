#ifndef HW3_BISON_CODE_H
#define HW3_BISON_CODE_H

#include "SymTable.hpp"
#include<string.h>

extern int yylineno;

void initSymTable(SymTable* symbol_table, vector<string>& predefined_func);
void checkMain(SymTable* symbol_table);

bool isNumeric(Type type);
Type checkIfBool(Type type);
Type checkByteVal(int val);

Type checkBinopExp(Type type1, Type type2);
Type checkRelopExp(Type type1, Type type2);
Type checkLogicExp(Type type1, Type type2);
Type checkConversion(Type target_type, Type type);

Type checkVarDeclaredBeforeUsed(SymTable* symbol_table, const char* name);
void checkVarNotDeclared(SymTable* symbol_table, const char* name);
void checkAssign(const char* id_name, Type id_type, Type exp_type);

void handleVarDec(SymTable* symbol_table, const char* name, Type type);
void handleVarInitialization(SymTable* symbol_table, const char* name, Type type, Type exp_type);
void handleVarReassign(SymTable* symbol_table, const char* name, Type exp_type);

SymTableEntry* checkIfLegalCall(SymTable* symbol_table, const char* func_name, vector<ExpInfo*>* args);

void checkBreakInWhile(vector<bool>& in_while);
void checkContinueInWhile(vector<bool>& in_while);

void checkEmptyRet(Type last_ret_type);
void checkExpRet(Type last_ret_type, Type exp_type);

bool checkFormalRedef(vector<string>& arg_names, string& new_arg_name);
int addFunc(SymTable* symbol_table, const char* func_name, Type ret_type, vector<ArgInfo*>* arg_list, bool is_override);
void addFuncScope(SymTable* symbol_table, vector<ArgInfo*>* arg_list);

void addScope(SymTable* symbol_table);
void delTopScope(SymTable* symbol_table);

string freshVar();
string freshGlobalStrVar();
void assignPlace(ExpInfo* exp_info);

void emitBinary(ExpInfo* target, ExpInfo* op1, ExpInfo* op2, string op);
void addDivByZeroCheck(ExpInfo* op2);
void emitLoadCommand(ExpInfo* target, SymTable* symbol_table, const char* id_name, string& base_ptr);
void emitNumToPlace(string& place, int val, Type type);
void emitStrToGlobal(ExpInfo* target, const char* str);

char* copyLabelStr();

void notAction(ExpInfo* target, ExpInfo* op);
void andAction(ExpInfo* target, ExpInfo* op1, ExpInfo* op2, const char* label);
void orAction(ExpInfo* target, ExpInfo* op1, ExpInfo* op2, const char* label);
void relopAction(ExpInfo* target, ExpInfo* op1, ExpInfo* op2, string op);
void boolAction(ExpInfo* target, bool val);
void conversionAction(ExpInfo* target, ExpInfo* op, Type target_type);

string getLLVMTypeStr(Type type);
string createFunc(const char* func_name, Type ret_type, vector<ArgInfo*>* arg_list, int func_idx);
string getDefaultVal(Type ret_type);
void closeFunc(Type ret_type, StatementInfo* st_info);

void emitVarDec(SymTable* symbol_table, const char* name, Type type, ExpInfo* exp_info, string& base_ptr);
void emitVarReassign(SymTable* symbol_table, const char* name, ExpInfo* exp_info, string& base_ptr);
void emitRet(ExpInfo* exp_info, Type ret_type);

void ifAction(StatementInfo* target, ExpInfo* exp_info, const char* label, StatementInfo* st_info);
void N_Action(StatementInfo* target);
void ifElseAction(StatementInfo* target, ExpInfo* if_exp, const char* true_label, StatementInfo* if_st, StatementInfo* N_st, char* false_label, StatementInfo* else_st);
void whileAction(StatementInfo* target, const char* cond_label, ExpInfo* while_exp, const char* body_label, StatementInfo* while_st);
void statementAction(StatementInfo* target, StatementInfo* st_info, const char* label);
void breakAction(StatementInfo* st_info);
void continueAction(StatementInfo* st_info);
void callAction(SymTable* symbol_table, ExpInfo* target, const char* func_name, vector<ExpInfo*>* args);
void expCallAction(ExpInfo* target);
void evalBoolExp(ExpInfo* exp_info);

void initCodeBuff();
void printCodeBuff();

#endif //HW3_BISON_CODE_H
