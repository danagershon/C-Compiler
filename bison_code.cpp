#include "bison_code.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>

void initSymTable(SymTable* symbol_table, vector<string>& predefined_func){
    symbol_table->addFuncSymbol(predefined_func[0].c_str(), VOID_TYPE, {STRING_TYPE}, false, 0);
    symbol_table->addFuncSymbol(predefined_func[1].c_str(), VOID_TYPE, {INT_TYPE}, false, 0);
}

void checkMain(SymTable* symbol_table){
    string name = "main";
    vector<SymTableEntry*> matches = symbol_table->getFuncSymbol(name.c_str(), {});

    // change conditions for error according to staff
    if(matches.empty() || matches[0]->type != VOID_TYPE){
        output::errorMainMissing();
        exit(0);
    }
}

bool isNumeric(Type type){
    return type == INT_TYPE || type == BYTE_TYPE;
}

Type checkIfBool(Type type){
    if(type != BOOL_TYPE) {
        output::errorMismatch(yylineno);
        exit(0);
    }

    return BOOL_TYPE;
}

Type checkByteVal(int val){
    if(val > 255){
        output::errorByteTooLarge(yylineno, to_string(val));
        exit(0);
    }

    return BYTE_TYPE;
}

Type checkBinopExp(Type type1, Type type2){
    if(!isNumeric(type1) || !isNumeric(type2)){
        output::errorMismatch(yylineno);
        exit(0);
    }
    if(type1 == INT_TYPE || type2 == INT_TYPE){
        return INT_TYPE;
    }

    return BYTE_TYPE;
}

Type checkRelopExp(Type type1, Type type2){
    if(!isNumeric(type1) || !isNumeric(type2)) {
        output::errorMismatch(yylineno);
        exit(0);
    }

    return BOOL_TYPE;
}

Type checkLogicExp(Type type1, Type type2){
    checkIfBool(type1);
    checkIfBool(type2);
    return BOOL_TYPE;
}

Type checkConversion(Type target_type, Type type){
    bool is_legal = isNumeric(target_type) && isNumeric(type);
    if(!is_legal){
        output::errorMismatch(yylineno);
        exit(0);
    }

    return target_type;
}

Type checkVarDeclaredBeforeUsed(SymTable* symbol_table, const char* name){
    SymTableEntry* symbol = symbol_table->getVarSymbol(name);
    if(!symbol){
        output::errorUndef(yylineno, string(name));
        exit(0);
    }

    return symbol->type;
}

void checkVarNotDeclared(SymTable* symbol_table, const char* name){
    if(symbol_table->getVarSymbol(name) || !symbol_table->getFuncsByName(name).empty()){
        output::errorDef(yylineno, string(name));
        exit(0);
    }
}

void checkAssign(const char* id_name, Type id_type, Type exp_type){
    bool is_legal = (id_type == exp_type) || (id_type == INT_TYPE && exp_type == BYTE_TYPE);
    if(!is_legal){
        output::errorMismatch(yylineno);
        exit(0);
    }
}

void handleVarDec(SymTable* symbol_table, const char* name, Type type){
    checkVarNotDeclared(symbol_table, name);
    symbol_table->addVarSymbol(name, type);
}

void handleVarInitialization(SymTable* symbol_table, const char* name, Type type, Type exp_type){
    checkVarNotDeclared(symbol_table, name);
    checkAssign(name, type, exp_type);
    symbol_table->addVarSymbol(name, type);
}

void handleVarReassign(SymTable* symbol_table, const char* name, Type exp_type){
    Type id_type = checkVarDeclaredBeforeUsed(symbol_table, name);
    checkAssign(name, id_type, exp_type);
}

SymTableEntry* checkIfLegalCall(SymTable* symbol_table, const char* func_name, vector<ExpInfo*>* args){
    vector<SymTableEntry*> matches = symbol_table->getFuncsByName(func_name);
    if(matches.empty()) {
        output::errorUndefFunc(yylineno, string(func_name));
        exit(0);
    }

    vector<Type> arg_types;
    if(args){
        for(auto& arg: *args){
            arg_types.push_back(arg->type);
        }
    }
    vector<SymTableEntry*> candidates = symbol_table->getFuncSymbol(func_name, arg_types);

    if(candidates.empty()) {
        output::errorPrototypeMismatch(yylineno, string(func_name));
        exit(0);
    }
    // new ambiguous rule
    if(candidates.size() > 1){
        output::errorAmbiguousCall(yylineno, string(func_name));
        exit(0);
    }

    return candidates[0];
}

void checkBreakInWhile(vector<bool>& in_while){
    if(in_while.empty()){
        output::errorUnexpectedBreak(yylineno);
        exit(0);
    }
}

void checkContinueInWhile(vector<bool>& in_while){
    if(in_while.empty()){
        output::errorUnexpectedContinue(yylineno);
        exit(0);
    }
}

void checkEmptyRet(Type last_ret_type){
    if(last_ret_type != VOID_TYPE){
        output::errorMismatch(yylineno);
        exit(0);
    }
}

void checkExpRet(Type last_ret_type, Type exp_type){
    if(last_ret_type == VOID_TYPE){
        output::errorMismatch(yylineno);
        exit(0);
    }

    bool is_legal = last_ret_type == exp_type || (last_ret_type == INT_TYPE && exp_type == BYTE_TYPE);
    if(!is_legal){
        output::errorMismatch(yylineno);
        exit(0);
    }
}

bool checkFormalRedef(vector<string>& arg_names, string& new_arg_name){
    for(auto& past_arg_name: arg_names){
        if(past_arg_name == new_arg_name){
            return true;
        }
    }
    return false;
}

int addFunc(SymTable* symbol_table, const char* func_name, Type ret_type, vector<ArgInfo*>* arg_list, bool is_override){
    // check if a variable already exists with the same name
    if(symbol_table->getVarSymbol(func_name)){
        output::errorDef(yylineno, string(func_name));
        exit(0);
    }
    // change conditions according to staff
    if(string(func_name) == "main" && is_override){
        output::errorMainOverride(yylineno);
        exit(0);
    }

    vector<Type> arg_types;
    vector<string> arg_names;

    if(arg_list){
        for(auto& arg_info: *arg_list){
            if(symbol_table->getVarSymbol(arg_info->arg_name.c_str()) || !symbol_table->getFuncsByName(arg_info->arg_name.c_str()).empty() || func_name == arg_info->arg_name || checkFormalRedef(arg_names, arg_info->arg_name)){
                output::errorDef(arg_info->arg_line, arg_info->arg_name);
                exit(0);
            }
            arg_types.push_back(arg_info->arg_type);
            arg_names.push_back(arg_info->arg_name);
        }
    }

    vector<SymTableEntry*> matches = symbol_table->getFuncsByName(func_name);

    // case of one other func that wasn't declared with override
    if(matches.size() == 1 && !matches[0]->is_override){
        if(is_override){
            output::errorFuncNoOverride(yylineno, string(func_name));
            exit(0);
        } else{ // both funcs declared without override
            output::errorDef(yylineno, string(func_name));
            exit(0);
        }
    }

    /* cases here:
     * - 1 other func with the same name that was declared with override
     *   => if curr wasn't declared with override: error
     * - 2+ or more other funcs with the same name => all should have been
     *   declared with override => if curr wasn't declared with override: error
     * - 0 other func with the same name => if is skipped
     * */
    if(!matches.empty() && !is_override){
        output::errorOverrideWithoutDeclaration(yylineno, string(func_name));
        exit(0);
    }

    // check for another func with the exact same prototype
    for(auto& match: matches){
        if(match->type == ret_type && match->arg_types == arg_types){
            output::errorDef(yylineno, string(func_name));
            exit(0);
        }
    }
    symbol_table->addFuncSymbol(func_name, ret_type, arg_types, is_override, int(matches.size()));
    return int(matches.size());
}

void addFuncScope(SymTable* symbol_table, vector<ArgInfo*>* arg_list){
    symbol_table->addNewScope();
    int offset = -1;

    if(arg_list){
        for(auto& arg_info: *arg_list){
            symbol_table->addArgSymbol(arg_info->arg_name.c_str(), arg_info->arg_type, offset);
            offset--;
        }
    }
}

void addScope(SymTable* symbol_table){
    symbol_table->addNewScope();
}

void delTopScope(SymTable* symbol_table){
    symbol_table->deleteTopScope();
}

string freshVar(){
    static int idx = 0;
    idx += 1;
    return "%t" + to_string(idx);
}

string freshGlobalStrVar(){
    static int idx = 0;
    idx += 1;
    return "@s" + to_string(idx);
}

void assignPlace(ExpInfo* exp_info){
    exp_info->place.assign(freshVar());
}

void addDivByZeroCheck(ExpInfo* op2){
    string is_zero = freshVar();
    if(op2->type == BYTE_TYPE){
        CodeBuffer::instance().emit(is_zero + " = icmp eq i8 0, " + op2->place);
    } else { // int type
        CodeBuffer::instance().emit(is_zero + " = icmp eq i32 0, " + op2->place);
    }
    int addr = CodeBuffer::instance().emit("br i1 " + is_zero + ", label @, label @");

    string exit_label = CodeBuffer::instance().genLabel();
    CodeBuffer::instance().emit("call void @divByZero()");
    CodeBuffer::instance().emit("unreachable");
    CodeBuffer::instance().bpatch(CodeBuffer::makelist({addr,FIRST}),exit_label);

    string continue_label = CodeBuffer::instance().genLabel();
    CodeBuffer::instance().bpatch(CodeBuffer::makelist({addr,SECOND}),continue_label);
}

void emitBinary(ExpInfo* target, ExpInfo* op1, ExpInfo* op2, string op){
    if(op == "div"){
        addDivByZeroCheck(op2);
        if(op1->type == BYTE_TYPE && op2->type == BYTE_TYPE){
            op = "udiv";
        } else {
            op = "sdiv";
        }
    }

    if(op1->type == INT_TYPE && op2->type == INT_TYPE){
        CodeBuffer::instance().emit(target->place + " = " + op + " i32 " + op1->place + ", " + op2->place);
    } else if(op1->type == INT_TYPE && op2->type == BYTE_TYPE){
        string temp = freshVar();
        CodeBuffer::instance().emit(temp + " = zext i8 " + op2->place + " to i32");
        CodeBuffer::instance().emit(target->place + " = " + op + " i32 " + op1->place + ", " + temp);
    } else if(op1->type == BYTE_TYPE && op2->type == INT_TYPE){
        string temp = freshVar();
        CodeBuffer::instance().emit(temp + " = zext i8 " + op1->place + " to i32");
        CodeBuffer::instance().emit(target->place + " = " + op + " i32 " + temp + ", " + op2->place);
    } else { // both byte
        CodeBuffer::instance().emit(target->place + " = " + op + " i8 " + op1->place + ", " + op2->place);
    }
}

void emitLoadCommand(ExpInfo* target, SymTable* symbol_table, const char* id_name, string& base_ptr){
    SymTableEntry* symbol = symbol_table->getVarSymbol(id_name);
    if(symbol->offset < 0){ // func arg
        if(symbol->type == BOOL_TYPE){
            CodeBuffer::instance().emit(target->place + " = add i1 0, " + "%arg" + to_string(abs(symbol->offset)));
        } else if(symbol->type == BYTE_TYPE){
            CodeBuffer::instance().emit(target->place + " = add i8 0, " + "%arg" + to_string(abs(symbol->offset)));
        } else {
            CodeBuffer::instance().emit(target->place + " = add i32 0, " + "%arg" + to_string(abs(symbol->offset)));
        }
    } else { // local variable
        string element_ptr = freshVar();
        CodeBuffer::instance().emit(element_ptr + " = getelementptr i32, i32* " + base_ptr + ", i32 " + to_string(symbol->offset));
        if(symbol->type == BOOL_TYPE){
            string temp = freshVar();
            CodeBuffer::instance().emit(temp + " = load i32, i32* " + element_ptr);
            CodeBuffer::instance().emit(target->place + " = trunc i32 " + temp + " to i1");
        } else if(symbol->type == BYTE_TYPE){
            string temp = freshVar();
            CodeBuffer::instance().emit(temp + " = load i32, i32* " + element_ptr);
            CodeBuffer::instance().emit(target->place + " = trunc i32 " + temp + " to i8");
        } else {
            CodeBuffer::instance().emit(target->place + " = load i32, i32* " + element_ptr);
        }
    }

    if(symbol->type == BOOL_TYPE){
        string cond = freshVar();
        CodeBuffer::instance().emit(cond + " = icmp eq i1 1, " + target->place);
        int addr = CodeBuffer::instance().emit("br i1 " + cond + ", label @, label @");
        target->truelist = CodeBuffer::makelist({addr,FIRST});
        target->falselist = CodeBuffer::makelist({addr,SECOND});
    }
}

void emitNumToPlace(string& place, int val, Type type){
    if(type == BYTE_TYPE){
        CodeBuffer::instance().emit(place + " = add i8 0, " + to_string(val));
    } else {
        CodeBuffer::instance().emit(place + " = add i32 0, " + to_string(val));
    }
}

void emitStrToGlobal(ExpInfo* target, const char* str){
    string str_var = freshGlobalStrVar();
    string str_type = "[" + to_string(strlen(str)-1) + " x i8]";
    string real_str = string(str);
    real_str.pop_back();

    CodeBuffer::instance().emitGlobal(str_var + " = internal constant " + str_type + " c" + real_str + "\\00\"");
    CodeBuffer::instance().emit(target->place + " = getelementptr " + str_type + ", " + str_type + "* " + str_var + ", i32 0, i32 0");
}

char* copyLabelStr(){
    /* might need to add br before new label in order to meet LLVM
     * basic block condition that it always ends with br or ret */
    int addr = CodeBuffer::instance().emit("br label @");
    string label = CodeBuffer::instance().genLabel();
    CodeBuffer::instance().bpatch(CodeBuffer::makelist({addr,FIRST}),label);
    char* copy = strdup(label.c_str());
    return copy;
}

void notAction(ExpInfo* target, ExpInfo* op){
    target->truelist = move(op->falselist);
    target->falselist = move(op->truelist);
}

void andAction(ExpInfo* target, ExpInfo* op1, ExpInfo* op2, const char* label){
    CodeBuffer::instance().bpatch(op1->truelist,string(label));
    target->truelist = move(op2->truelist);
    target->falselist = CodeBuffer::merge(op1->falselist,op2->falselist);
}

void orAction(ExpInfo* target, ExpInfo* op1, ExpInfo* op2, const char* label){
    CodeBuffer::instance().bpatch(op1->falselist,string(label));
    target->truelist = CodeBuffer::merge(op1->truelist,op2->truelist);
    target->falselist = move(op2->falselist);
}

void relopAction(ExpInfo* target, ExpInfo* op1, ExpInfo* op2, string op){
    string cond = freshVar();
    if(op1->type == INT_TYPE && op2->type == INT_TYPE){
        CodeBuffer::instance().emit(cond + " = icmp " + op + " i32 " + op1->place + ", " + op2->place);
    } else if(op1->type == INT_TYPE && op2->type == BYTE_TYPE){
        string temp = freshVar();
        CodeBuffer::instance().emit(temp + " = zext i8 " + op2->place + " to i32");
        CodeBuffer::instance().emit(cond + " = icmp " + op + " i32 " + op1->place + ", " + temp);
    } else if(op1->type == BYTE_TYPE && op2->type == INT_TYPE){
        string temp = freshVar();
        CodeBuffer::instance().emit(temp + " = zext i8 " + op1->place + " to i32");
        CodeBuffer::instance().emit(cond + " = icmp " + op + " i32 " + temp + ", " + op2->place);
    } else { // both byte
        CodeBuffer::instance().emit(cond + " = icmp " + op + " i8 " + op1->place + ", " + op2->place);
    }

    int addr = CodeBuffer::instance().emit("br i1 " + cond + ", label @, label @");
    target->truelist = CodeBuffer::makelist({addr,FIRST});
    target->falselist = CodeBuffer::makelist({addr,SECOND});
}

void boolAction(ExpInfo* target, bool val){
    int addr = CodeBuffer::instance().emit("br label @");
    if(val){
        target->truelist = CodeBuffer::makelist({addr,FIRST});
        target->falselist.clear();
    } else {
        target->falselist = CodeBuffer::makelist({addr,FIRST});
        target->truelist.clear();
    }
}

void conversionAction(ExpInfo* target, ExpInfo* op, Type target_type){
    if(target_type == BYTE_TYPE && op->type ==INT_TYPE){
        CodeBuffer::instance().emit(target->place + " = trunc i32 " + op->place + " to i8");
    } else if(target_type == INT_TYPE && op->type == INT_TYPE){
        CodeBuffer::instance().emit(target->place + " = add i32 0, " + op->place);
    } else if(target_type == INT_TYPE && op->type == BYTE_TYPE){
        CodeBuffer::instance().emit(target->place + " = zext i8 " + op->place + " to i32");
    } else { // both byte
        CodeBuffer::instance().emit(target->place + " = add i8 0, " + op->place);
    }
}

string getLLVMTypeStr(Type type){
    if(type == VOID_TYPE){
        return "void";
    }
    else if(type == STRING_TYPE){
        return "i8*";
    } else if(type == BOOL_TYPE){
        return "i1";
    } else if(type == BYTE_TYPE){
        return "i8";
    }
    return "i32";
}

string createFunc(const char* func_name, Type ret_type, vector<ArgInfo*>* arg_list, int func_idx){
    string real_func_name;

    if(string(func_name) == "main" && ret_type == VOID_TYPE && !arg_list){
        real_func_name = "@" + string(func_name);
    } else {
        real_func_name = "@" + string(func_name) + to_string(func_idx);
    }

    string ret_type_str = getLLVMTypeStr(ret_type);

    string args_str;
    int arg_idx = 1;
    if(arg_list){
        for(auto& arg_info: *arg_list){
            args_str += getLLVMTypeStr(arg_info->arg_type) + " %arg" + to_string(arg_idx) + ", ";
            arg_idx++;
        }
        args_str.pop_back();
        args_str.pop_back();
    }

    CodeBuffer::instance().emit("define " + ret_type_str + " " + real_func_name + "(" + args_str + ") {");
    string base_ptr = freshVar();
    CodeBuffer::instance().emit(base_ptr + " = alloca i32, i32 50");
    return base_ptr;
}

string getDefaultVal(Type ret_type){
    if(ret_type == VOID_TYPE){
        return "";
    }
    return "0";
}

void closeFunc(Type ret_type, StatementInfo* st_info){
    int addr = CodeBuffer::instance().emit("br label @");
    string end_label = CodeBuffer::instance().genLabel();
    CodeBuffer::instance().bpatch(CodeBuffer::makelist({addr,FIRST}),end_label);

    CodeBuffer::instance().emit("ret " + getLLVMTypeStr(ret_type) + " " + getDefaultVal(ret_type));
    CodeBuffer::instance().emit("}");
    CodeBuffer::instance().bpatch(st_info->nextlist,end_label);
}

void emitVarDec(SymTable* symbol_table, const char* name, Type type, ExpInfo* exp_info, string& base_ptr){
    string val;
    if(!exp_info){
        val = getDefaultVal(type);
    } else {
        val = exp_info->place;
    }

    SymTableEntry* symbol = symbol_table->getVarSymbol(name);
    string element_ptr = freshVar();
    CodeBuffer::instance().emit(element_ptr + " = getelementptr i32, i32* " + base_ptr + ", i32 " + to_string(symbol->offset));

    if(exp_info && exp_info->type == BOOL_TYPE){
        string temp = freshVar();
        CodeBuffer::instance().emit(temp + " = zext i1 " + val + " to i32");
        val = temp;
    }
    if(exp_info && exp_info->type == BYTE_TYPE){
        string temp = freshVar();
        CodeBuffer::instance().emit(temp + " = zext i8 " + val + " to i32");
        val = temp;
    }

    CodeBuffer::instance().emit("store i32 " + val + ", i32* " + element_ptr);
}

void emitVarReassign(SymTable* symbol_table, const char* name, ExpInfo* exp_info, string& base_ptr){
    SymTableEntry* symbol = symbol_table->getVarSymbol(name);
    string element_ptr = freshVar();
    CodeBuffer::instance().emit(element_ptr + " = getelementptr i32, i32* " + base_ptr + ", i32 " + to_string(symbol->offset));

    string val = exp_info->place;

    if(exp_info->type == BOOL_TYPE){
        string temp = freshVar();
        CodeBuffer::instance().emit(temp + " = zext i1 " + val + " to i32");
        val = temp;
    }
    if(exp_info->type == BYTE_TYPE){
        string temp = freshVar();
        CodeBuffer::instance().emit(temp + " = zext i8 " + val + " to i32");
        val = temp;
    }

    CodeBuffer::instance().emit("store i32 " + val + ", i32* " + element_ptr);
}

void emitRet(ExpInfo* exp_info, Type ret_type){
    if(!exp_info){
        CodeBuffer::instance().emit("ret void");
    } else {
        if(exp_info->type == BYTE_TYPE && ret_type == INT_TYPE){
            string temp = freshVar();
            CodeBuffer::instance().emit(temp + " = zext i8 " + exp_info->place + " to i32");
            CodeBuffer::instance().emit("ret " + getLLVMTypeStr(ret_type) + " " + temp);
        } else {
            CodeBuffer::instance().emit("ret " + getLLVMTypeStr(ret_type) + " " + exp_info->place);
        }
    }
}

void ifAction(StatementInfo* target, ExpInfo* exp_info, const char* label, StatementInfo* st_info){
    CodeBuffer::instance().bpatch(exp_info->truelist,string(label));
    target->nextlist = CodeBuffer::merge(exp_info->falselist,st_info->nextlist);
    target->breaklist = move(st_info->breaklist);
    target->continuelist = move(st_info->continuelist);
}

void N_Action(StatementInfo* target){
    int addr = CodeBuffer::instance().emit("br label @");
    target->nextlist = CodeBuffer::makelist({addr,FIRST});
}

void ifElseAction(StatementInfo* target, ExpInfo* if_exp, const char* true_label, StatementInfo* if_st, StatementInfo* N_st, char* false_label, StatementInfo* else_st){
    CodeBuffer::instance().bpatch(if_exp->truelist,string(true_label));
    CodeBuffer::instance().bpatch(if_exp->falselist,string(false_label));
    target->nextlist = CodeBuffer::merge(CodeBuffer::merge(if_st->nextlist,N_st->nextlist),else_st->nextlist);
    target->breaklist = CodeBuffer::merge(if_st->breaklist,else_st->breaklist);
    target->continuelist = CodeBuffer::merge(if_st->continuelist,else_st->continuelist);
}

void whileAction(StatementInfo* target, const char* cond_label, ExpInfo* while_exp, const char* body_label, StatementInfo* while_st){
    CodeBuffer::instance().bpatch(while_st->nextlist,string(cond_label));
    CodeBuffer::instance().bpatch(while_st->continuelist,string(cond_label));
    CodeBuffer::instance().bpatch(while_exp->truelist,string(body_label));

    target->nextlist = CodeBuffer::merge(while_exp->falselist,while_st->breaklist);
    CodeBuffer::instance().emit("br label %" + string(cond_label));
}

void statementAction(StatementInfo* target, StatementInfo* st_info, const char* label){
    CodeBuffer::instance().bpatch(st_info->nextlist,string(label));
    target->breaklist = CodeBuffer::merge(target->breaklist,st_info->breaklist);
    target->continuelist = CodeBuffer::merge(target->continuelist,st_info->continuelist);
}

void breakAction(StatementInfo* st_info){
    int addr = CodeBuffer::instance().emit("br label @");
    st_info->breaklist = CodeBuffer::makelist({addr,FIRST});
}

void continueAction(StatementInfo* st_info){
    int addr = CodeBuffer::instance().emit("br label @");
    st_info->continuelist = CodeBuffer::makelist({addr,FIRST});
}

void callAction(SymTable* symbol_table, ExpInfo* target, const char* func_name, vector<ExpInfo*>* args){
    SymTableEntry* match = checkIfLegalCall(symbol_table,func_name,args);
    target->type = match->type;

    string real_func_name;
    if(string(func_name) == "main" && !args){
        real_func_name = "@" + string(func_name);
    } else {
        real_func_name = "@" + string(func_name) + to_string(match->func_idx);
    }

    string args_str;
    if(args){
        for(unsigned long i = 0; i < args->size(); i++){
            if(match->arg_types[i] == args->at(i)->type){
                args_str += getLLVMTypeStr(args->at(i)->type) + " " + args->at(i)->place + ", ";
            } else {
                string temp = freshVar();
                CodeBuffer::instance().emit(temp + " = zext i8 " + args->at(i)->place + " to i32");
                args_str += getLLVMTypeStr(match->arg_types[i]) + " " + temp + ", ";
            }
        }
        args_str.pop_back();
        args_str.pop_back();
    }
    if(target->type != VOID_TYPE) {
        CodeBuffer::instance().emit(target->place + " = call " + getLLVMTypeStr(match->type) + " " + real_func_name + "(" + args_str + ")");
    } else {
        CodeBuffer::instance().emit("call " + getLLVMTypeStr(match->type) + " " + real_func_name + "(" + args_str + ")");
    }
}

void expCallAction(ExpInfo* target){
    if(target->type == BOOL_TYPE){
        string cond = freshVar();
        CodeBuffer::instance().emit(cond + " = icmp eq i1 1, " + target->place);
        int addr = CodeBuffer::instance().emit("br i1 " + cond + ", label @, label @");
        target->truelist = CodeBuffer::makelist({addr,FIRST});
        target->falselist = CodeBuffer::makelist({addr,SECOND});
    }
}

void evalBoolExp(ExpInfo* exp_info){
    if(exp_info->type != BOOL_TYPE){
        return;
    }
    exp_info->place.assign(freshVar());

    string true_label = CodeBuffer::instance().genLabel();
    int addr1 = CodeBuffer::instance().emit("br label @");

    string false_label = CodeBuffer::instance().genLabel();
    int addr2 = CodeBuffer::instance().emit("br label @");

    string assign_label = CodeBuffer::instance().genLabel();
    CodeBuffer::instance().emit(exp_info->place + " = phi i1 [1, %" + true_label + "], [0, %" + false_label + "]");

    CodeBuffer::instance().bpatch(exp_info->truelist,true_label);
    CodeBuffer::instance().bpatch(exp_info->falselist,false_label);

    CodeBuffer::instance().bpatch(CodeBuffer::makelist({addr1,FIRST}),assign_label);
    CodeBuffer::instance().bpatch(CodeBuffer::makelist({addr2,FIRST}),assign_label);
}

void initCodeBuff(){
    vector<string> print_func = {"declare i32 @printf(i8*, ...)\n",
                                 "declare void @exit(i32)\n",
                                 "@.int_specifier = constant [4 x i8] c\"%d\\0A\\00\"\n",
                                 "@.str_specifier = constant [4 x i8] c\"%s\\0A\\00\"\n",
                                 "@zero_error = constant [23 x i8] c\"Error division by zero\\00\"\n",
                                 "\n",
                                 "define void @printi0(i32) {\n",
                                 "    %spec_ptr = getelementptr [4 x i8], [4 x i8]* @.int_specifier, i32 0, i32 0\n",
                                 "    call i32 (i8*, ...) @printf(i8* %spec_ptr, i32 %0)\n",
                                 "    ret void\n",
                                 "}\n",
                                 "\n",
                                 "define void @print0(i8*) {\n",
                                 "    %spec_ptr = getelementptr [4 x i8], [4 x i8]* @.str_specifier, i32 0, i32 0\n",
                                 "    call i32 (i8*, ...) @printf(i8* %spec_ptr, i8* %0)\n",
                                 "    ret void\n",
                                 "}\n",
                                 "\n",
                                 "define void @divByZero() {\n",
                                 "    %zero_error_ptr = getelementptr [23 x i8], [23 x i8]* @zero_error, i32 0, i32 0\n",
                                 "    call void @print0(i8* %zero_error_ptr)\n",
                                 "    call void @exit(i32 0)\n",
                                 "    ret void\n",
                                 "}"};
    for(auto& str: print_func){
        CodeBuffer::instance().emitGlobal(str);
    }
}

void printCodeBuff(){
    CodeBuffer::instance().printGlobalBuffer();
    CodeBuffer::instance().printCodeBuffer();
}