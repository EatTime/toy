//
//  ch2_toy.cpp
//  Toy
//
//  Created by 柴榆 on 2021/8/12.
//

#include "ch2_toy.h"

using namespace llvm;

int Toy::get_token()
{
    while (isspace(LastChar))
        LastChar = fgetc(file);
    
    if (isalpha(LastChar))
    {
        Identifier_string = LastChar;
        while (isalnum((LastChar = fgetc(file))))
            Identifier_string += LastChar;
        
        if (Identifier_string == "def")
            return DEF_TOKEN;
        
        return IDENTIFIER_TOKEN;
    }
    
    if (isdigit(LastChar))
    {
        std::string NumStr;
        do
        {
            NumStr += LastChar;
            LastChar = fgetc(file);
        } while (isdigit(LastChar));
        
        Numeric_Val = strtod(NumStr.c_str(), 0);
        return NUMERIC_TOKEN;
    }
    
    if (LastChar == '#')
    {
        do
            LastChar = fgetc(file);
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');
        
        if (LastChar != EOF)
            return get_token();
    }
    
    if (LastChar == ',')
    {
        LastChar = fgetc(file);
        return TAKE_TOKEN;
    }
    
    if (LastChar == EOF)
        return EOF_TOKEN;
    
    int ThisChar = LastChar;
    LastChar = fgetc(file);
    return ThisChar;
}

int Toy::next_token() { return Current_token = get_token(); }

int Toy::getBinOpPrecedence()
{
    if (!isascii(Current_token))
        return -1;
    
    int TokPrec = Operator_Precedence[Current_token];
    if (TokPrec <= 0)
        return -1;
    return TokPrec;
}

BaseAST *Toy::identifier_parser()
{
    std::string IdName = Identifier_string;
    
    next_token();
    
    if (Current_token != '(')
        return new VariableAST(IdName);
    
    next_token();
    
    std::vector<BaseAST *> Args;
    if (Current_token != ')')
    {
        while (1)
        {
            BaseAST *Arg = expression_parser();
            if (!Arg)
                return 0;
            Args.push_back(Arg);
            
            if (Current_token == ')')
                break;
            
            if (Current_token != ',')
                return 0;
            next_token();
        }
    }
    next_token();
    
    return new FunctionCallAST(IdName, Args);
}

BaseAST *Toy::numeric_parser()
{
    BaseAST *Result = new NumericAST(Numeric_Val);
    next_token();
    return Result;
}

BaseAST *Toy::paran_parser()
{
    next_token();
    BaseAST *V = expression_parser();
    if (!V)
        return 0;
    
    if (Current_token != ')')
        return 0;
    return V;
}

BaseAST *Toy::Base_Parser()
{
    switch (Current_token)
    {
        default:
            return 0;
        case IDENTIFIER_TOKEN:
            return identifier_parser();
        case NUMERIC_TOKEN:
            return numeric_parser();
        case '(':
            return paran_parser();
    }
}

BaseAST *Toy::binary_op_parser(int Old_Prec, BaseAST *LHS)
{
    while (1)
    {
        int Operator_Prec = getBinOpPrecedence();
        
        if (Operator_Prec < Old_Prec)
            return LHS;
        
        int BinOp = Current_token;
        next_token();
        
        BaseAST *RHS = Base_Parser();
        if (!RHS)
            return 0;
        
        int Next_Prec = getBinOpPrecedence();
        if (Operator_Prec < Next_Prec)
        {
            RHS = binary_op_parser(Operator_Prec + 1, RHS);
            if (RHS == 0)
                return 0;
        }
        
        LHS = new BinaryAST(std::to_string(BinOp), LHS, RHS);
    }
}

BaseAST *Toy::expression_parser()
{
    BaseAST *LHS = Base_Parser();
    if (!LHS)
        return 0;
    return binary_op_parser(0, LHS);
}

FunctionDeclAST *Toy::func_decl_parser()
{
    if (Current_token != IDENTIFIER_TOKEN)
        return 0;
    
    std::string FnName = Identifier_string;
    next_token();
    
    if (Current_token != '(')
        return 0;
    
    std::vector<std::string> Function_Argument_Names;
    int flag;
    do
    {
        flag = next_token();
        if (flag == IDENTIFIER_TOKEN)
            Function_Argument_Names.push_back(Identifier_string);
        if (Function_Argument_Names.size() > 11)
            return 0;
    } while (flag == IDENTIFIER_TOKEN || flag == TAKE_TOKEN);
    if (Current_token != ')')
        return 0;
    
    next_token();
    
    return new FunctionDeclAST(FnName, Function_Argument_Names);
}

FunctionDefnAST *Toy::func_defn_parser()
{
    next_token();
    FunctionDeclAST *Decl = func_decl_parser();
    if (Decl == 0)
        return 0;
    
    if (BaseAST *Body = expression_parser())
        return new FunctionDefnAST(Decl, Body);
    return 0;
}

FunctionDefnAST *Toy::top_level_parser()
{
    if (BaseAST *E = expression_parser())
    {
        FunctionDeclAST *Func_Decl =
        new FunctionDeclAST("", std::vector<std::string>());
        return new FunctionDefnAST(Func_Decl, E);
    }
    return 0;
}

void Toy::init_precedence()
{
    Operator_Precedence['-'] = 1;
    Operator_Precedence['+'] = 2;
    Operator_Precedence['/'] = 3;
    Operator_Precedence['*'] = 4;
}

static Module *Module_Ob;
static LLVMContext MyGlobalContext;
static IRBuilder<> Builder(MyGlobalContext);

Value *NumericAST::Codegen()
{
    return ConstantInt::get(Type::getInt32Ty(MyGlobalContext), numeric_val);
}

Value *VariableAST::Codegen()
{
    Value *V = Named_Values[Var_Name];
    return V ? V : 0;
}

Value *BinaryAST::Codegen()
{
    Value *L = LHS->Codegen();
    Value *R = RHS->Codegen();
    if (L == 0 || R == 0)
        return 0;
    
    switch (atoi(Bin_Operator.c_str()))
    {
        case '+':
            return Builder.CreateAdd(L, R, "addtmp");
        case '-':
            return Builder.CreateSub(L, R, "subtmp");
        case '*':
            return Builder.CreateMul(L, R, "multmp");
        case '/':
            return Builder.CreateUDiv(L, R, "divtmp");
        default:
            return 0;
    }
}

Value *FunctionCallAST::Codegen()
{
    Function *CalleeF = Module_Ob->getFunction(Function_Callee);
    
    std::vector<Value *> ArgsV;
    for (unsigned i = 0, e = Function_Arguments.size(); i != e; ++i)
    {
        ArgsV.push_back(Function_Arguments[i]->Codegen());
        if (ArgsV.back() == 0)
            return 0;
    }
    
    return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

Function *FunctionDeclAST::Codegen()
{
    std::vector<Type *> Integers(Arguments.size(),
                                 Type::getInt32Ty(MyGlobalContext));
    FunctionType *FT =
    FunctionType::get(Type::getInt32Ty(MyGlobalContext), Integers, false);
    Function *F =
    Function::Create(FT, Function::ExternalLinkage, Func_Name, Module_Ob);
    
    if (F->getName() != Func_Name)
    {
        F->eraseFromParent();
        F = Module_Ob->getFunction(Func_Name);
        
        if (!F->empty())
            return 0;
        
        if (F->arg_size() != Arguments.size())
            return 0;
    }
    
    unsigned Idx = 0;
    for (Function::arg_iterator Arg_It = F->arg_begin(); Idx != Arguments.size();
         ++Arg_It, ++Idx)
    {
        Arg_It->setName(Arguments[Idx]);
        Named_Values[Arguments[Idx]] = Arg_It;
    }
    
    return F;
}

Function *FunctionDefnAST::Codegen()
{
    Named_Values.clear();
    
    Function *TheFunction = Func_Decl->Codegen();
    if (TheFunction == 0)
        return 0;
    
    BasicBlock *BB = BasicBlock::Create(MyGlobalContext, "entry", TheFunction);
    Builder.SetInsertPoint(BB);
    
    if (Value *RetVal = Body->Codegen())
    {
        Builder.CreateRet(RetVal);
        verifyFunction(*TheFunction);
        return TheFunction;
    }
    
    TheFunction->eraseFromParent();
    return 0;
}

void Toy::HandleDefn()
{
    if (FunctionDefnAST *F = func_defn_parser())
    {
        if (Function *LF = F->Codegen())
        {
        }
    }
    else
    {
        next_token();
    }
}

void Toy::HandleTopExpression()
{
    if (FunctionDefnAST *F = top_level_parser())
    {
        if (Function *LF = F->Codegen())
        {
        }
    }
    else
    {
        next_token();
    }
}

void Toy::Driver()
{
    while (1)
    {
        switch (Current_token)
        {
            case EOF_TOKEN:
                return;
            case ';':
                next_token();
                break;
            case DEF_TOKEN:
                HandleDefn();
                break;
            default:
                HandleTopExpression();
                break;
        }
    }
}
int Toy::run()
{
    LLVMContext &Context = MyGlobalContext;
    init_precedence();
    
    file = fopen(argv[1], "r");
    if (file == 0)
    {
        printf("Could not open file\n");
    }
    
    next_token();
    Module_Ob = new Module("my compiler", Context);
    Driver();
    Module_Ob->print(llvm::outs(), nullptr);
    return 0;
}
