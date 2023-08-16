#ifndef HW3_ATTRIBUTES_H
#define HW3_ATTRIBUTES_H

#include <string>
#include <vector>
#include "bp.hpp"

enum Type {
    STRING_TYPE = 0,
    INT_TYPE = 1,
    BYTE_TYPE = 2,
    BOOL_TYPE = 3,
    VOID_TYPE = 4,
};

class ArgInfo {
public:
    string arg_name;
    Type arg_type;
    int arg_line;
};

class ExpInfo {
public:
    Type type;
    string place;
    vector<pair<int,BranchLabelIndex>> truelist;
    vector<pair<int,BranchLabelIndex>> falselist;
};

class StatementInfo {
public:
    vector<pair<int,BranchLabelIndex>> nextlist;
    vector<pair<int,BranchLabelIndex>> breaklist;
    vector<pair<int,BranchLabelIndex>> continuelist;
};

#endif //HW3_ATTRIBUTES_H
