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
    error("expected '%s'", str);
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
    if (*p == '+' || *p == '-') {
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

  // 声明一个全局main段，同时也是程序入口段
  printf("  .globl main\n");
  // main段标签
  printf("main:\n");

  // 这里我们将算式分解为 num (op num) (op num)...的形式
  printf("  li a0, %d\n", getNumber(tok));

  // 解析 (op num)
  tok = tok->next;
  while (tok->kind != TK_EOF) {
    if (equal(tok, "+")) {
      tok = tok->next;
      printf("  addi a0, a0, %d\n", getNumber(tok));
      tok = tok->next;
      continue;
    }

    tok = skip(tok, "-");
    printf("  addi a0, a0, -%d\n", getNumber(tok));
    tok = tok->next;
  }

  // 生成程序结束指令
  printf("  ret\n");

  return 0;
}
