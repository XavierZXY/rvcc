#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief 当前输入的字符串
 */
static char *current_input;

/**
 * @brief 为终结符设置种类
 */
typedef enum {
  TK_RESERVED, // 符号
  TK_NUM,      // 整数
  TK_EOF,      // 文件结束
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

/**
 * @brief generate a new Token
 * @param  kind
 * @param  start
 * @param  end
 * @return Token*
 */
static Token *newToken(TokenKind kind, char *start, char *end) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->loc = start;
  tok->len = end - start;
  return tok;
}

/**
 * @brief print error message and exit
 * @param  fmt
 * @param  ...
 */
static void error(char *fmt, ...) {
  va_list ap;
  // va_start，初始化一个va_list，用于获取可变参数
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

/**
 * @brief 出错位置
 * @param  loc
 * @param  fmt
 * @param  ap
 */
static void verrotAt(char *loc, char *fmt, va_list ap) {
  fprintf(stderr, "%s\n", current_input);
  // 获取错误位置
  int pos = loc - current_input;
  // 输出错误位置
  fprintf(stderr, "%*s", pos, "");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
}

/**
 * @brief 解析错误位置
 * @param  loc
 * @param  fmt
 * @param  ...
 */
static void errorAt(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verrotAt(loc, fmt, ap);
  va_end(ap);
  exit(1);
}

/**
 * @brief token解析错误位置
 * @param  tok
 * @param  fmt
 * @param  ...
 */
static void errorTok(Token *tok, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verrotAt(tok->loc, fmt, ap);
  va_end(ap);
  exit(1);
}

/**
 * @brief compare Token and str
 * @param  tok
 * @param  str
 * @return true
 * @return false
 */
static bool equal(Token *tok, char *str) {
  // memcmp，比较两个内存区域的内容
  // 比较按照字典序，LHS<RHS回负值，LHS=RHS返回0，LHS>RHS返回正值
  return memcmp(tok->loc, str, tok->len) == 0 && str[tok->len] == '\0';
}

/**
 * @brief skip token if it is expected symbol `-`
 * @param  tok
 * @param  str
 * @return Token*
 */
static Token *skip(Token *tok, char *str) {
  if (!equal(tok, str)) {
    errorTok(tok, "expected '%s'", str);
  }
  return tok->next;
}

/**
 * @brief Get the Number object
 * @param  tok
 * @return int
 */
static int getNumber(Token *tok) {
  if (tok->kind != TK_NUM) {
    errorTok(tok, "expected a number");
  }

  return tok->val;
}

/**
 * @brief 解析过程
 * @param  p
 * @return Token*
 */
static Token *tokenize() {
  char *p = current_input;
  Token head = {};
  Token *cur = &head;
  while (*p) {
    if (isspace(*p)) {
      ++p;
      continue;
    }

    // 解析数字
    if (isdigit(*p)) {
      cur = cur->next = newToken(TK_NUM, p, p);
      const char *old_p = p;
      cur->val = strtoul(p, &p, 10);
      cur->len = p - old_p;
      continue;
    }

    // 解析符号
    if (ispunct(*p)) {
      cur = cur->next = newToken(TK_RESERVED, p, p + 1);
      ++p;
      continue;
    }

    errorAt(p, "invalid token");
  }

  // 解析结束
  cur->next = newToken(TK_EOF, p, p);

  return head.next;
}

/* 生成AST（抽象语法树）*/

// AST的节点种类
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // \*
  ND_DIV, // /
  ND_NUM, // 整数
} NodeKind;

// AST的节点结构体
typedef struct Node Node;
struct Node {
  NodeKind kind; // 节点种类
  Node *lhs;     // 左子节点
  Node *rhs;     // 右子节点
  int val;       // 节点值
};

/**
 * @brief create a new Node
 * @param  kind
 * @return Node*
 */
static Node *newNode(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

/**
 * @brief create a binary Node
 * @param  kind
 * @param  lhs
 * @param  rhs
 * @return Node*
 */
static Node *newBinary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = newNode(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

/**
 * @brief create a new number Node
 * @param  val
 * @return Node*
 */
static Node *newNum(int val) {
  Node *node = newNode(ND_NUM);
  node->val = val;
  return node;
}

/* 语法解析 */

// 加减解析
static Node *expr(Token **rest, Token *tok);
// 乘除解析
static Node *mul(Token **rest, Token *tok);
// 数字解析
static Node *primary(Token **rest, Token *tok);

/**
 * @brief 加减解析
 * @param  rest
 * @param  tok
 * @return Node*
 */
static Node *expr(Token **rest, Token *tok) {
  Node *node = mul(&tok, tok);
  while (true) {
    if (equal(tok, "+")) {
      node = newBinary(ND_ADD, node, mul(&tok, tok->next));
      continue;
    }

    if (equal(tok, "-")) {
      node = newBinary(ND_SUB, node, mul(&tok, tok->next));
      continue;
    }
    *rest = tok;
    return node;
  }
}

/**
 * @brief 乘除解析
 * @param  rest
 * @param  tok
 * @return Node*
 */
static Node *mul(Token **rest, Token *tok) {
  Node *node = primary(&tok, tok);
  while (true) {
    if (equal(tok, "*")) {
      node = newBinary(ND_MUL, node, primary(&tok, tok->next));
      continue;
    }

    if (equal(tok, "/")) {
      node = newBinary(ND_DIV, node, primary(&tok, tok->next));
      continue;
    }

    *rest = tok;
    return node;
  }
}

/**
 * @brief 括号和数字解析
 * @param  rest
 * @param  tok
 * @return Node*
 */
static Node *primary(Token **rest, Token *tok) {

  // "(" expr ")"
  if (equal(tok, "(")) {
    Node *node = expr(&tok, tok->next);
    *rest = skip(tok, ")");
    return node;
  }

  // number
  if (tok->kind == TK_NUM) {
    Node *node = newNum(tok->val);
    *rest = tok->next;
    return node;
  }

  errorTok(tok, "expected an expression");
  return NULL;
}

/* 语义分析与代码生成 */
static int Depth;
/**
 * @brief 压栈，将结果临时压入栈中备用
 * 当前栈指针的地址就是sp，将a0的值压入栈
 * sp为栈指针，栈反向向下增长，64位下，8个字节为一个单位，所以sp-8
 * 不使用寄存器存储的原因是因为需要存储的值的数量是变化的。
 */
static void push(void) {
  printf("  addi sp, sp, -8\n");
  printf("  sd a0, 0(sp)\n");
  Depth++;
}

/**
 * @brief stack pop function
 * @param  reg
 */
static void pop(char *reg) {
  printf("  ld %s, 0(sp)\n", reg);
  printf("  addi sp, sp, 8\n");
  Depth--;
}

/**
 * @brief genrate expression
 * @param  node
 */
static void genExpr(Node *node) {
  if (node->kind == ND_NUM) {
    printf("  li a0, %d\n", node->val);
    return;
  }

  genExpr(node->rhs);
  push();
  genExpr(node->lhs);
  pop("a1");

  switch (node->kind) {
  case ND_ADD:
    printf("  add a0, a0, a1\n");
    return;
  case ND_SUB:
    printf("  sub a0, a0, a1\n");
    return;
  case ND_MUL:
    printf("  mul a0, a0, a1\n");
    return;
  case ND_DIV:
    printf("  div a0, a0, a1\n");
    return;
  default:
    break;
  }

  error("invalid expression");
}

int main(int Argc, char **Argv) {
  // 判断传入程序的参数是否为2个，Argv[0]为程序名称，Argv[1]为传入的第一个参数
  if (Argc != 2) {
    // 异常处理，提示参数数量不对。
    // fprintf，格式化文件输出，往文件内写入字符串
    // stderr，异常文件（Linux一切皆文件），用于往屏幕显示异常信息
    // %s，字符串
    error("%s: invalid number of arguments", Argv[0]);
  }

  // 解析参数
  current_input = Argv[1];
  Token *tok = tokenize();

  Node *node = expr(&tok, tok);
  if (tok->kind != TK_EOF) {
    errorTok(tok, "extra token");
  }

  // 声明一个全局main段，同时也是程序入口段
  printf("  .globl main\n");
  // main段标签
  printf("main:\n");

  // 遍历AST
  genExpr(node);

  // 生成程序结束指令
  printf("  ret\n");

  assert(Depth == 0);

  return 0;
}
