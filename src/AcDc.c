#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "header.h"
#define MAX_STACK_SIZE 100 //used for stack

Token IDs[23]; //added by zhuqi
int no = 0;    //added by zhuqi
int first = 0; //added by zhuqi, constant folding for plus&minus

int stack[MAX_STACK_SIZE];         //used for stack
float stack_float[MAX_STACK_SIZE]; //used for float stack
int top = -1;                      //used for stack
int top_float = -1;                //used for float stack

int main(int argc, char *argv[])
{
    FILE *source, *target, *test;
    Program program;
    SymbolTable symtab;

    if (argc == 3)
    {
        source = fopen(argv[1], "r");
        target = fopen(argv[2], "w");
        test = fopen(argv[3], "w");
        if (!source)
        {
            printf("can't open the source file\n");
            exit(2);
        }
        else if (!target)
        {
            printf("can't open the target file\n");
            exit(2);
        }
        else
        {
            program = parser(source);
            fclose(source);
            symtab = build(program);
            check(&program, &symtab);
            gencode(program, target);
        }
    }
    else
    {
        printf("Usage: %s source_file target_file\n", argv[0]);
    }

    return 0;
}

/********************************************* 
  Stack Operation 
 *********************************************/
void init_stack()
{
    top = -1;
}

void push(int data)
{
    if (top >= MAX_STACK_SIZE - 1)
    {
        printf("stack over flow!\n");
    }
    else
    {
        top++;
        stack[top] = data;
    }
}

int pop()
{
    int data;
    if (top <= -1)
    {
        printf("stack under flow!\n");
    }
    else
    {
        data = stack[top];
        top--;
        return data;
    }
}

void init_stack_float()
{
    top_float = -1;
}

void push_float(float data)
{
    if (top_float >= MAX_STACK_SIZE - 1)
    {
        printf("stack float over flow!\n");
    }
    else
    {
        top_float++;
        stack_float[top_float] = data;
    }
}

float pop_float()
{
    float data;
    if (top_float <= -1)
    {
        printf("stack float under flow!\n");
    }
    else
    {
        data = stack_float[top_float];
        top_float--;
        return data;
    }
}

/********************************************* 
  Scanning 
 *********************************************/
Token getNumericToken(FILE *source, char c)
{
    Token token;
    int i = 0;

    while (isdigit(c))
    {
        token.tok[i++] = c;
        c = fgetc(source);
    }

    if (c != '.')
    {
        ungetc(c, source);
        token.tok[i] = '\0';
        token.type = IntValue;
        return token;
    }

    token.tok[i++] = '.';

    c = fgetc(source);
    if (!isdigit(c))
    {
        ungetc(c, source);
        printf("Expect a digit : %c\n", c);
        exit(1);
    }

    while (isdigit(c))
    {
        token.tok[i++] = c;
        c = fgetc(source);
    }

    ungetc(c, source);
    token.tok[i] = '\0';
    token.type = FloatValue;
    return token;
}

Token scanner(FILE *source)
{
    char c;
    Token token;

    while (!feof(source))
    {
        c = fgetc(source);

        while (isspace(c))
            c = fgetc(source);

        if (isdigit(c))
            return getNumericToken(source, c);

        token.tok[0] = c; //變量名只有一個字母，這邊需要修改
        token.tok[1] = '\0';

        if (islower(c))
        {
            int i = 0;
            while (islower(c))
            {
                token.tok[i++] = c;
                c = fgetc(source);
            }
            ungetc(c, source);
            token.tok[i] = '\0';
            if (i == 1)
            {
                if (strcmp(token.tok, "f") == 0)
                {
                    token.type = FloatDeclaration;
                    return token;
                }
                else if (strcmp(token.tok, "i") == 0)
                {
                    token.type = IntegerDeclaration;
                    return token;
                }
                else if (strcmp(token.tok, "p") == 0)
                {
                    token.type = PrintOp;
                    return token;
                }
            }
            token.type = Alphabet;
            for (i = 0; i < 23; i++)
            {
                if (strcmp(token.tok, IDs[i].tok) == 0)
                {
                    return token;
                }
            }
            IDs[no] = token;
            no++;
            return token;
        }

        switch (c)
        {
        case '=':
            token.type = AssignmentOp;
            return token;
        case '+':
            token.type = PlusOp;
            return token;
        case '-':
            token.type = MinusOp;
            return token;
        case '*':
            token.type = MulOp; //有乘除
            return token;
        case '/':
            token.type = DivOp; //
            return token;
        case EOF:
            token.type = EOFsymbol;
            token.tok[0] = '\0'; //EOF用空字符表示
            return token;
        default:
            printf("Invalid character : %c\n", c);
            exit(1);
        }
    }

    token.tok[0] = '\0'; //文件流遇到末尾
    token.type = EOFsymbol;
    return token;
}

/********************************************************
  Parsing
 *********************************************************/
Declaration parseDeclaration(FILE *source, Token token)
{
    Token token2;
    switch (token.type)
    {
    case FloatDeclaration:
    case IntegerDeclaration:
        token2 = scanner(source);
        if (strcmp(token2.tok, "f") == 0 ||
            strcmp(token2.tok, "i") == 0 ||
            strcmp(token2.tok, "p") == 0)
        {
            printf("Syntax Error: %s cannot be used as id\n", token2.tok);
            exit(1);
        }
        return makeDeclarationNode(token, token2);
    default:
        printf("Syntax Error: Expect Declaration %s\n", token.tok);
        exit(1);
    }
}

Declarations *parseDeclarations(FILE *source)
{
    Token token = scanner(source);
    Declaration decl;
    Declarations *decls;
    int i = 0, j;
    switch (token.type)
    {
    case FloatDeclaration:
    case IntegerDeclaration:
        decl = parseDeclaration(source, token);
        decls = parseDeclarations(source);
        return makeDeclarationTree(decl, decls);
    case PrintOp:
    case Alphabet:
        //ungetc(token.tok[0], source);//not enough, the token now has long length variable name.
        while (token.tok[i++] != '\0')
            ;
        for (j = i - 2; j > -1; j--)
        {
            ungetc(token.tok[j], source);
        }
        return NULL;
    case EOFsymbol:
        return NULL;
    default:
        printf("Syntax Error: Expect declarations %s\n", token.tok);
        exit(1);
    }
}

Expression *constfold(FILE *source, Expression *lvalue) //constant folding for plus&minus
{
    Token token = scanner(source);
    Token next_token = scanner(source);
    Expression *value = (Expression *)malloc(sizeof(Expression));
    value->leftOperand = value->rightOperand = NULL;
    int i = 0, j; //for feed back
    if (token.type == IntValue || token.type == FloatValue)
    {
        switch (token.type)
        {
        case IntValue:
            (value->v).type = IntConst;
            (value->v).val.ivalue = atoi(token.tok);
            break;
        case FloatValue:
            (value->v).type = FloatConst;
            (value->v).val.fvalue = atof(token.tok);
            break;
        }
        //feed back next_token...
        i = 0;
        while (next_token.tok[i++] != '\0')
            ;
        for (j = i - 2; j > -1; j--)
            ungetc(next_token.tok[j], source);
        return constfold(source, value);
    }
    else if ((token.type == MinusOp || token.type == PlusOp) &&
             (next_token.type == IntValue || next_token.type == FloatValue))
    {
        int result;
        float fresult;
        int left;
        int right;
        float fleft;
        float fright;
        if ((lvalue->v).type == IntConst)
        {
            left = (lvalue->v).val.ivalue;
            if (next_token.type == IntValue)
            {
                right = atoi(next_token.tok);
                if (token.type == MinusOp)
                    result = left - right;
                else
                    result = left + right;
                (value->v).type = IntConst;
                (value->v).val.ivalue = result;
                return constfold(source, value);
            }
            else if (next_token.type == FloatValue)
            {
                fright = atof(next_token.tok);
                if (token.type == MinusOp)
                    fresult = left - fright;
                else
                    fresult = left + fright;
                (value->v).type = FloatConst;
                (value->v).val.fvalue = fresult;
                return constfold(source, value);
            }
        }
        else if ((lvalue->v).type == FloatConst)
        {
            fleft = (lvalue->v).val.fvalue;
            if (next_token.type == IntValue)
            {
                right = atoi(next_token.tok);
                if (token.type == MinusOp)
                    fresult = fleft - right;
                else
                    fresult = fleft + right;
                (value->v).type = FloatConst;
                (value->v).val.fvalue = fresult;
                return constfold(source, value);
            }
            else if (next_token.type == FloatValue)
            {
                fright = atof(next_token.tok);
                if (token.type == MinusOp)
                    fresult = fleft - fright;
                else
                    fresult = fleft + fright;
                (value->v).type = FloatConst;
                (value->v).val.fvalue = fresult;
                return constfold(source, value);
            }
        }
    }
    else
    { //feed back token and next_token...
        i = 0;
        while (next_token.tok[i++] != '\0')
            ;
        for (j = i - 2; j > -1; j--)
            ungetc(next_token.tok[j], source);
        ungetc(' ', source);
        i = 0;
        while (token.tok[i++] != '\0')
            ;
        for (j = i - 2; j > -1; j--)
            ungetc(token.tok[j], source);
        return lvalue;
    }
}

Expression *parseValue(FILE *source) //這邊要做常數折疊
{
    Token token = scanner(source);
    Token next_token = scanner(source); //判斷是否是MulNode or DivNode or MinusNode or PlusNode
    Expression *value = (Expression *)malloc(sizeof(Expression));
    Expression *expr;
    int i = 0, j;
    value->leftOperand = value->rightOperand = NULL;

    switch (token.type)
    {
    case Alphabet:
        (value->v).type = Identifier;
        for (int i = 0; i < 23; i++)
        {
            if (strcmp(IDs[i].tok, token.tok) == 0)
            {
                (value->v).val.id = 'a' + i; //maybe need change
                break;
            }
        }
        //(value->v).val.id = token.tok[0];
        break;
    case IntValue:
        (value->v).type = IntConst;
        (value->v).val.ivalue = atoi(token.tok);
        break;
    case FloatValue:
        (value->v).type = FloatConst;
        (value->v).val.fvalue = atof(token.tok);
        break;
    default:
        printf("Syntax Error: Expect Identifier or a Number %s\n", token.tok);
        exit(1);
    }

    if (next_token.type == MulOp)
    {
        expr = (Expression *)malloc(sizeof(Expression));
        (expr->v).type = MulNode;
        (expr->v).val.op = Mul;
        expr->leftOperand = value;
        expr->rightOperand = parseValue(source);
        return expr;
    }
    else if (next_token.type == DivOp)
    {
        expr = (Expression *)malloc(sizeof(Expression));
        (expr->v).type = DivNode;
        (expr->v).val.op = Div;
        expr->leftOperand = value;
        expr->rightOperand = parseValue(source);
        return expr;
    }
    else
    {
        //ungetc(next_token.tok[0],source);//hidden trouble
        while (next_token.tok[i++] != '\0')
            ;
        for (j = i - 2; j > -1; j--)
            ungetc(next_token.tok[j], source);
        return value;
    }
}

Expression *parseExpressionTail(FILE *source, Expression *lvalue)
{
    Token token = scanner(source);
    Expression *expr;
    int i = 0, j;
    switch (token.type)
    {
    case PlusOp:
        expr = (Expression *)malloc(sizeof(Expression));
        (expr->v).type = PlusNode;
        (expr->v).val.op = Plus;
        expr->leftOperand = lvalue;
        expr->rightOperand = parseValue(source);
        return parseExpressionTail(source, expr);
    case MinusOp:
        expr = (Expression *)malloc(sizeof(Expression));
        (expr->v).type = MinusNode;
        (expr->v).val.op = Minus;
        expr->leftOperand = lvalue;
        expr->rightOperand = parseValue(source);
        return parseExpressionTail(source, expr);
    case Alphabet:
    case PrintOp:
        //ungetc(token.tok[0], source);
        while (token.tok[i++] != '\0')
            ;
        for (j = i - 2; j > -1; j--)
        {
            ungetc(token.tok[j], source);
        }
        return lvalue;
    case EOFsymbol:
        return lvalue;
    default:
        printf("Tail Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
        exit(1);
    }
}

Expression *parseExpression(FILE *source, Expression *lvalue)
{
    Token token = scanner(source);
    Expression *expr;
    int i = 0, j;
    switch (token.type)
    {
    case PlusOp:
        expr = (Expression *)malloc(sizeof(Expression));
        (expr->v).type = PlusNode;
        (expr->v).val.op = Plus;
        expr->leftOperand = lvalue;
        expr->rightOperand = parseValue(source);
        return parseExpressionTail(source, expr);
    case MinusOp:
        expr = (Expression *)malloc(sizeof(Expression));
        (expr->v).type = MinusNode;
        (expr->v).val.op = Minus;
        expr->leftOperand = lvalue;
        expr->rightOperand = parseValue(source);
        return parseExpressionTail(source, expr);
    case Alphabet:
    case PrintOp:
        //ungetc(token.tok[0], source);
        while (token.tok[i++] != '\0')
            ;
        for (j = i - 2; j > -1; j--)
        {
            ungetc(token.tok[j], source);
        }
        return NULL;
    case EOFsymbol:
        return NULL;
    default:
        printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
        exit(1);
    }
}

Statement parseStatement(FILE *source, Token token) //need change
{
    Token next_token;
    Expression *constant,*value, *expr;

    switch (token.type)
    {
    case Alphabet:
        next_token = scanner(source);
        if (next_token.type == AssignmentOp)
        {
            constant=constfold(source,NULL);
            if(constant==NULL){
                value = parseValue(source);
                expr = parseExpression(source, value);
                for (int i = 0; i < 23; i++)
                {
                    if (strcmp(IDs[i].tok, token.tok) == 0)
                    {
                        return makeAssignmentNode('a' + i, value, expr);
                    }
                }
            }
            else{
                expr=parseExpression(source, constant);
                for (int i = 0; i < 23; i++)
                {
                    if (strcmp(IDs[i].tok, token.tok) == 0)
                    {
                        return makeAssignmentNode('a' + i, constant, expr);
                    }
                }
            }
            //return makeAssignmentNode(token.tok[0], value, expr);
        }
        else
        {
            printf("Syntax Error: Expect an assignment op %s\n", next_token.tok);
            exit(1);
        }
    case PrintOp:
        next_token = scanner(source);
        if (next_token.type == Alphabet)
        {
            for (int i = 0; i < 23; i++)
            {
                if (strcmp(IDs[i].tok, next_token.tok) == 0)
                {
                    return makePrintNode('a' + i);
                }
            }
        }
        //return makePrintNode(next_token.tok[0]);
        else
        {
            printf("Syntax Error: Expect an identifier %s\n", next_token.tok);
            exit(1);
        }
        break;
    default:
        printf("Syntax Error: Expect a statement %s\n", token.tok);
        exit(1);
    }
}

Statements *parseStatements(FILE *source)
{

    Token token = scanner(source);
    Statement stmt;
    Statements *stmts;

    switch (token.type)
    {
    case Alphabet:
    case PrintOp:
        stmt = parseStatement(source, token);
        stmts = parseStatements(source);
        return makeStatementTree(stmt, stmts);
    case EOFsymbol:
        return NULL;
    default:
        printf("Syntax Error: Expect statements %s\n", token.tok);
        exit(1);
    }
}

/*********************************************************************
  Build AST
 **********************************************************************/
Declaration makeDeclarationNode(Token declare_type, Token identifier)
{
    Declaration tree_node;

    switch (declare_type.type)
    {
    case FloatDeclaration:
        tree_node.type = Float;
        break;
    case IntegerDeclaration:
        tree_node.type = Int;
        break;
    default:
        break;
    }
    for (int i = 0; i < 23; i++)
    {
        if (strcmp(IDs[i].tok, identifier.tok) == 0)
        {
            tree_node.name = 'a' + i;
            break;
        }
    }
    //tree_node.name = identifier.tok[0];
    return tree_node;
}

Declarations *makeDeclarationTree(Declaration decl, Declarations *decls)
{
    Declarations *new_tree = (Declarations *)malloc(sizeof(Declarations));
    new_tree->first = decl;
    new_tree->rest = decls;

    return new_tree;
}

Statement makeAssignmentNode(char id, Expression *v, Expression *expr_tail)
{
    Statement stmt;
    AssignmentStatement assign;

    stmt.type = Assignment;
    assign.id = id;
    if (expr_tail == NULL)
        assign.expr = v;
    else
        assign.expr = expr_tail;
    stmt.stmt.assign = assign;

    return stmt;
}

Statement makePrintNode(char id)
{
    Statement stmt;
    stmt.type = Print;
    stmt.stmt.variable = id;

    return stmt;
}

Statements *makeStatementTree(Statement stmt, Statements *stmts)
{
    Statements *new_tree = (Statements *)malloc(sizeof(Statements));
    new_tree->first = stmt;
    new_tree->rest = stmts;

    return new_tree;
}

/* parser */
Program parser(FILE *source)
{
    Program program;

    program.declarations = parseDeclarations(source);
    program.statements = parseStatements(source);

    return program;
}

/********************************************************
  Build symbol table
 *********************************************************/
void InitializeTable(SymbolTable *table)
{
    int i;

    for (i = 0; i < 26; i++)
        table->table[i] = Notype;
}

void add_table(SymbolTable *table, char c, DataType t)
{
    int index = (int)(c - 'a');

    if (table->table[index] != Notype)
        printf("Error : id %c has been declared\n", c); //error
    table->table[index] = t;
}

SymbolTable build(Program program)
{
    SymbolTable table;
    Declarations *decls = program.declarations;
    Declaration current;

    InitializeTable(&table);

    while (decls != NULL)
    {
        current = decls->first;
        add_table(&table, current.name, current.type);
        decls = decls->rest;
    }

    return table;
}

/********************************************************************
  Type checking
 *********************************************************************/

void convertType(Expression *old, DataType type) //attention!這裏插入了一個類型轉換節點
{
    if (old->type == Float && type == Int)
    {
        printf("error : can't convert float to integer\n");
        return;
    }
    if (old->type == Int && type == Float)
    {
        Expression *tmp = (Expression *)malloc(sizeof(Expression));
        if (old->v.type == Identifier)
            printf("convert to float %c \n", old->v.val.id);
        else
            printf("convert to float %d \n", old->v.val.ivalue);
        /*tmp->v = old->v;
        tmp->leftOperand = old->leftOperand;
        tmp->rightOperand = old->rightOperand;
        tmp->type = old->type;

        Value v;
        v.type = IntToFloatConvertNode;
        v.val.op = IntToFloatConvert;
        old->v = v;
        old->type = Int;
        old->leftOperand = tmp;
        old->rightOperand = NULL;*/
    }
}

DataType generalize(Expression *left, Expression *right)
{
    if (left->type == Float || right->type == Float)
    {
        printf("generalize : float\n");
        return Float;
    }
    printf("generalize : int\n");
    return Int;
}

DataType lookup_table(SymbolTable *table, char c) //變量使用前必須聲明
{
    int id = c - 'a';
    if (table->table[id] != Int && table->table[id] != Float)
        printf("Error : identifier %c is not declared\n", c); //error
    return table->table[id];
}

void checkexpression(Expression *expr, SymbolTable *table)
{
    char c;
    if (expr->leftOperand == NULL && expr->rightOperand == NULL)
    {
        switch (expr->v.type)
        {
        case Identifier:
            c = expr->v.val.id;
            printf("identifier : %c\n", c);
            expr->type = lookup_table(table, c);
            break;
        case IntConst:
            printf("constant : int\n");
            expr->type = Int;
            break;
        case FloatConst:
            printf("constant : float\n");
            expr->type = Float;
            break;
            //case PlusNode: case MinusNode: case MulNode: case DivNode:
        default:
            break;
        }
    }
    else
    {
        Expression *left = expr->leftOperand;
        Expression *right = expr->rightOperand;

        checkexpression(left, table);
        checkexpression(right, table);

        DataType type = generalize(left, right);
        convertType(left, type);  //left->type = type;//converto
        convertType(right, type); //right->type = type;//converto
        expr->type = type;
    }
}

void checkstmt(Statement *stmt, SymbolTable *table)
{
    if (stmt->type == Assignment)
    {
        AssignmentStatement assign = stmt->stmt.assign;
        printf("assignment : %c \n", assign.id);
        checkexpression(assign.expr, table);
        stmt->stmt.assign.type = lookup_table(table, assign.id);
        if (assign.expr->type == Float && stmt->stmt.assign.type == Int)
        {
            printf("error : can't convert float to integer\n");
        }
        else
        {
            convertType(assign.expr, stmt->stmt.assign.type);
        }
    }
    else if (stmt->type == Print)
    {
        printf("print : %c \n", stmt->stmt.variable);
        lookup_table(table, stmt->stmt.variable);
    }
    else
        printf("error : statement error\n"); //error
}

void check(Program *program, SymbolTable *table)
{
    Statements *stmts = program->statements;
    while (stmts != NULL)
    {
        checkstmt(&stmts->first, table);
        stmts = stmts->rest;
    }
}

/***********************************************************************
  Code generation
 ************************************************************************/
void fprint_op(FILE *target, ValueType op)
{
    switch (op)
    {
    case MinusNode:
        fprintf(target, "-\n");
        break;
    case PlusNode:
        fprintf(target, "+\n");
        break;
    case MulNode: //added by zhuqi
        fprintf(target, "*\n");
        break;
    case DivNode: //added by zhuqi
        fprintf(target, "/\n");
        break;
    default:
        fprintf(target, "Error in fprintf_op ValueType = %d\n", op);
        break;
    }
}

void constant_op(ValueType op, int mode) //print to a char*/string
{
    if (mode == 1)
    {
        int right_operand = pop();
        int left_operand = pop();
        int result;
        switch (op)
        {
        case MulNode: //added by zhuqi
            result = left_operand * right_operand;
            push(result);
            break;
        case DivNode: //added by zhuqi
            result = left_operand / right_operand;
            push(result);
            break;
        }
    }
    else if (mode == 2)
    {
        float right_operand = pop_float();
        float left_operand = pop_float();
        float result;
        switch (op)
        {
        case MulNode: //added by zhuqi
            result = left_operand * right_operand;
            push_float(result);
            break;
        case DivNode: //added by zhuqi
            result = left_operand / right_operand;
            push_float(result);
            break;
        }
    }
}

/*返回0表示不能進行常數折疊，返回1表示能進行int常數折疊,返回2表示能進行float常數折疊*/
int check_constant(Expression *expr)
{
    if ((expr->v).type == FloatConst)
        return 2;
    else if ((expr->v).type == IntConst)
        return 1;
    else if ((expr->leftOperand->v).type == FloatConst)
    {
        if (check_constant(expr->rightOperand))
            return 2;
        else
            return 0;
    }
    else if ((expr->leftOperand->v).type == IntConst)
    {
        if (check_constant(expr->rightOperand))
            return check_constant(expr->rightOperand);
        else
            return 0;
    }
    return 0;
}

void calculat_constant(Expression *expr, int mode)
{
    if ((expr->v).type == MulNode || (expr->v).type == DivNode)
    { //added by zhuqi
        if (expr->leftOperand != NULL)
        {
            calculat_constant(expr->leftOperand, mode);
        }
        if ((expr->rightOperand->v).type == MulNode || (expr->rightOperand->v).type == DivNode)
        { //連續的乘除
            calculat_constant(expr->rightOperand->leftOperand, mode);
            expr->rightOperand->leftOperand = NULL;
            constant_op((expr->v).type, mode);
            calculat_constant(expr->rightOperand, mode);
        }
        else
        {
            calculat_constant(expr->rightOperand, mode);
            constant_op((expr->v).type, mode);
        }
    }
    else if (expr->leftOperand == NULL)
    {
        switch ((expr->v).type)
        {
        case IntConst:
            if (mode == 1)
                push((expr->v).val.ivalue);
            else if (mode == 2)
                push_float((expr->v).val.ivalue);
            break;
        case FloatConst:
            push_float((expr->v).val.fvalue);
            break;
        default:
            printf("Error In calculat_constant_expr. (expr->v).type=%d\n", (expr->v).type);
            break;
        }
    }
}

void fprint_expr(FILE *target, Expression *expr)
{
    if ((expr->v).type == MulNode || (expr->v).type == DivNode)
    { //added by zhuqi
        //constant folding
        int check_result;
        check_result = check_constant(expr);
        if (check_result == 1)
        { //常數串的乘除運算，折疊成int
            calculat_constant(expr, 1);
            fprintf(target, "%d\n", pop());
            return;
        }
        if (check_result == 2)
        { //常數串的乘除運算，折疊成float
            calculat_constant(expr, 2);
            fprintf(target, "%f\n", pop_float());
            return;
        }
        if (expr->leftOperand != NULL)
        {
            fprint_expr(target, expr->leftOperand);
        }
        if ((expr->rightOperand->v).type == MulNode || (expr->rightOperand->v).type == DivNode)
        { //連續的乘除
            fprint_expr(target, expr->rightOperand->leftOperand);
            expr->rightOperand->leftOperand = NULL;
            fprint_op(target, (expr->v).type);
            fprint_expr(target, expr->rightOperand);
        }
        else
        {
            fprint_expr(target, expr->rightOperand);
            fprint_op(target, (expr->v).type);
        }
    }
    else if (expr->leftOperand == NULL)
    {
        switch ((expr->v).type)
        {
        case Identifier:
            fprintf(target, "l%c\n", (expr->v).val.id);
            break;
        case IntConst:
            fprintf(target, "%d\n", (expr->v).val.ivalue);
            break;
        case FloatConst:
            fprintf(target, "%f\n", (expr->v).val.fvalue);
            break;
        default:
            fprintf(target, "Error In fprint_left_expr. (expr->v).type=%d\n", (expr->v).type);
            break;
        }
    }
    else
    {
        fprint_expr(target, expr->leftOperand);
        if (expr->rightOperand == NULL)
        {
            fprintf(target, "5k\n");
        }
        else
        {
            fprint_expr(target, expr->rightOperand);
            fprint_op(target, (expr->v).type);
        }
    }
}

void gencode(Program prog, FILE *target)
{
    Statements *stmts = prog.statements;
    Statement stmt;

    while (stmts != NULL)
    {
        stmt = stmts->first;
        switch (stmt.type)
        {
        case Print:
            fprintf(target, "l%c\n", stmt.stmt.variable);
            fprintf(target, "p\n");
            break;
        case Assignment:
            fprint_expr(target, stmt.stmt.assign.expr);
            /*
                   if(stmt.stmt.assign.type == Int){
                   fprintf(target,"0 k\n");
                   }
                   else if(stmt.stmt.assign.type == Float){
                   fprintf(target,"5 k\n");
                   }*/
            fprintf(target, "s%c\n", stmt.stmt.assign.id);
            fprintf(target, "0 k\n");
            break;
        }
        stmts = stmts->rest;
    }
}

/***************************************
  For our debug,
  you can omit them.
 ****************************************/
void print_expr(Expression *expr)
{
    if (expr == NULL)
        return;
    else
    {
        print_expr(expr->leftOperand);
        printf("\nmiddle type: %d\n", (expr->v).type);
        switch ((expr->v).type)
        {
        case Identifier:
            printf("%c ", (expr->v).val.id);
            break;
        case IntConst:
            printf("%d ", (expr->v).val.ivalue);
            break;
        case FloatConst:
            printf("%f ", (expr->v).val.fvalue);
            break;
        case PlusNode:
            printf("+ ");
            break;
        case MinusNode:
            printf("- ");
            break;
        case MulNode:
            printf("* ");
            break;
        case DivNode:
            printf("/ ");
            break;
        case IntToFloatConvertNode:
            printf("(float) ");
            break;
        default:
            printf("error ");
            break;
        }
        print_expr(expr->rightOperand);
    }
}

void test_parser(FILE *source)
{
    Declarations *decls;
    Statements *stmts;
    Declaration decl;
    Statement stmt;
    Program program = parser(source);

    decls = program.declarations;

    while (decls != NULL)
    {
        decl = decls->first;
        if (decl.type == Int)
            printf("i ");
        if (decl.type == Float)
            printf("f ");
        printf("%c ", decl.name);
        decls = decls->rest;
    }

    stmts = program.statements;

    while (stmts != NULL)
    {
        stmt = stmts->first;
        if (stmt.type == Print)
        {
            printf("p %c ", stmt.stmt.variable);
        }

        if (stmt.type == Assignment)
        {
            printf("%c = ", stmt.stmt.assign.id);
            print_expr(stmt.stmt.assign.expr);
        }
        stmts = stmts->rest;
    }
}
