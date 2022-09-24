//
//  ch2_toy.hpp
//  toy
//
//  Created by 柴榆 on 2021/8/12.
//

#ifndef ch2_toy_h
#define ch2_toy_h

#include <stdio.h>
#include <cctype>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

enum Token_Type
{
    EOF_TOKEN = 0,
    DEF_TOKEN,
    IDENTIFIER_TOKEN,
    NUMERIC_TOKEN,
    TAKE_TOKEN
};

class BaseAST
{
public:
    virtual ~BaseAST() {}
    virtual Value *Codegen() = 0;
};

class NumericAST : public BaseAST
{
    int numeric_val;
    
public:
    NumericAST(int val) : numeric_val(val) {}
    virtual Value *Codegen();
};

class VariableAST : public BaseAST
{
    std::string Var_Name;
    
public:
    VariableAST(const std::string &name) : Var_Name(name) {}
    virtual Value *Codegen();
};

class BinaryAST : public BaseAST
{
    std::string Bin_Operator;
    BaseAST *LHS, *RHS;
    
public:
    BinaryAST(std::string op, BaseAST *lhs, BaseAST *rhs)
    : Bin_Operator(op), LHS(lhs), RHS(rhs) {}
    virtual Value *Codegen();
};

class FunctionCallAST : public BaseAST
{
    std::string Function_Callee;
    std::vector<BaseAST *> Function_Arguments;
    
public:
    FunctionCallAST(const std::string &callee, std::vector<BaseAST *> &args)
    : Function_Callee(callee), Function_Arguments(args) {}
    virtual Value *Codegen();
};

class FunctionDeclAST
{
    std::string Func_Name;
    std::vector<std::string> Arguments;
    
public:
    FunctionDeclAST(const std::string &name, const std::vector<std::string> &args)
    : Func_Name(name), Arguments(args){};
    Function *Codegen();
};

class FunctionDefnAST
{
    FunctionDeclAST *Func_Decl;
    BaseAST *Body;
    
public:
    FunctionDefnAST(FunctionDeclAST *proto, BaseAST *body)
    : Func_Decl(proto), Body(body) {}
    Function *Codegen();
};

class Toy
{
    FILE *file;
    int LastChar = ' ';
    static std::string Identifier_string;
    static int Numeric_Val;
    static int Current_token;
    static std::map<char, int> Operator_Precedence;
    static std::map<std::string, Value *> Named_Values;
    
public:
    Toy() { init_precedence(); }
    int get_token();
    static int next_token();
    static int getBinOpPrecedence();
    static BaseAST *expression_parser();
    static BaseAST *identifier_parser();
    static BaseAST *numeric_parser();
    static BaseAST *paran_parser();
    static BaseAST *Base_Parser();
    static BaseAST *binary_op_parser(int Old_Prec, BaseAST *LHS);
    static FunctionDeclAST *func_decl_parser();
    static FunctionDefnAST *func_defn_parser();
    static FunctionDefnAST *top_level_parser();
    static void init_precedence();
    static void HandleDefn();
    static void HandleTopExpression();
    static void Driver();
    int run();
};

#endif /* ch2_toy_hpp */
