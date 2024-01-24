#include "rvcc.h"

/**
 * @brief 当前输入的字符串
 */
char *current_input;

/**
 * @brief generate a new Token
 * @param  kind
 * @param  start
 * @param  end
 * @return Token*
 */
Token *newToken(TokenKind kind, char *start, char *end) {
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
void error(char *fmt, ...) {
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
void errorAt(char *loc, char *fmt, ...) {
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
void errorTok(Token *tok, char *fmt, ...) {
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
bool equal(Token *tok, char *str) {
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
Token *skip(Token *tok, char *str) {
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
 * @brief  判断str是否以sub_str开头
 * @param  str
 * @param  sub_str
 * @return true false
 */
static bool startsWith(char *str, char *sub_str) {
  return strncmp(str, sub_str, strlen(sub_str)) == 0;
}

/**
 * @brief 判断运算符
 * @param  p
 * @return int
 */
static int readPunct(char *p) {
  // 2字符运算符
  if (startsWith(p, "==") || startsWith(p, "!=") || startsWith(p, "<=") ||
      startsWith(p, ">=")) {
    return 2;
  }

  // 1字符运算符
  return ispunct(*p) ? 1 : 0;
}

/**
 * @brief 解析过程
 * @param  p
 * @return Token*
 */
Token *tokenize(char *p) {
  current_input = p;
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
    int punct_len = readPunct(p);
    if (punct_len) {
      cur = cur->next = newToken(TK_PUNCT, p, p + punct_len);
      p += punct_len;
      continue;
    }

    errorAt(p, "invalid token");
  }

  // 解析结束
  cur->next = newToken(TK_EOF, p, p);

  return head.next;
}