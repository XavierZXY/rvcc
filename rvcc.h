// 使用POSIX.1标准
// 使用了strndup函数
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief 为终结符设置种类
 */
typedef enum TokenKind {
  TK_IDENT, // 标识符
  TK_PUNCT, // 符号
  TK_KEYWORD, // 关键字
  TK_NUM,   // 整数
  TK_EOF,   // 文件结束
} TokenKind;

/**
 * @brief 终结符结构体
 */
typedef struct Token Token;
struct Token {
  TokenKind kind; // Token种类
  Token *next;    // 下一个Token
  int val;        // Token的值
  char *loc;      // Token的字符串位置
  int len;        // Token的长度
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void errorTok(Token *tok, char *fmt, ...);
bool equal(Token *tok, char *str);
Token *skip(Token *tok, char *str);
Token *tokenize(char *str);

/* 生成AST（抽象语法树）*/

typedef struct Obj Obj;
typedef struct Node Node;

// AST的节点种类
typedef enum NodeKind {
  ND_ADD,       // `+`
  ND_SUB,       // `-`
  ND_MUL,       // `*`
  ND_DIV,       // `/`
  ND_NEG,       // `-` nagetive
  ND_EQ,        // `==`
  ND_NE,        // `!=`
  ND_LT,        // `<`
  ND_LE,        // `<=`
  ND_ASSIGN,    // `=`
  ND_VAR,       // 局部变量
  ND_BLOCK,     // { ... }，代码块
  ND_EXPR_STMT, // 表达式语句
  ND_NUM,       // 整数
  ND_RETURN,    // return
} NodeKind;

// AST的节点结构体
struct Node {
  NodeKind kind; // 节点种类
  Node *next;    // 下一个节点, 用于表达式语句
  Node *lhs;     // 左子节点
  Node *rhs;     // 右子节点
  Node *body;
  Obj *var;      // 变量
  int val;       // 节点值
};

// 本地变量
struct Obj {
  Obj *next;  // 指向下一对象
  char *name; // 变量名
  int offset; // fp的偏移量
};

// 函数
typedef struct Function Function;
struct Function {
  Node *body;     // 函数体
  Obj *locals;    // 本地变量
  int stack_size; // 栈大小
};

/**
 * @brief 语法解析
 * @param  tok
 * @return Node*
 */
Function *parse(Token *tok);

/* 语义分析与代码生成 */

/**
 * @brief 代码生成函数
 * @param  node
 */
void codegen(Function *prog);
