#include "rvcc.h"

/**
 * @brief create a new Node
 * @param  kind
 * @return Node*
 */
static Node *newNode(NodeKind kind, Token *tok) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->tok = tok;
  return node;
}

/**
 * @brief 新建一个单插树
 * @param  kind
 * @param  expr
 * @return Node*
 */
static Node *newUnary(NodeKind kind, Node *expr, Token *tok) {
  Node *node = newNode(kind, tok);
  node->lhs = expr;
  return node;
}

/**
 * @brief create a binary Node
 * @param  kind
 * @param  lhs
 * @param  rhs
 * @return Node*
 */
static Node *newBinary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
  Node *node = newNode(kind, tok);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

/**
 * @brief creat a new variable Node
 * @param  name
 * @return Node*
 */
static Node *newVarNode(Obj *var, Token *tok) {
  Node *node = newNode(ND_VAR, tok);
  node->var = var;
  return node;
}

/**
 * @brief create a new number Node
 * @param  val
 * @return Node*
 */
static Node *newNum(int val, Token *tok) {
  Node *node = newNode(ND_NUM, tok);
  node->val = val;
  return node;
}

// local variable list
Obj *locals;

/**
 * @brief create a new local variable
 * @param  tok
 * @return Obj*
 */
static Obj *newLocalVar(char *name) {
  Obj *var = calloc(1, sizeof(Obj));
  var->name = name;
  var->next = locals;
  locals = var;
  return var;
}

/**
 * @brief find a local variable
 * @param  name
 * @return Obj*
 */
static Obj *findVar(Token *tok) {
  for (Obj *var = locals; var; var = var->next) {
    if (strlen(var->name) == tok->len &&
        !strncmp(tok->loc, var->name, tok->len)) {
      return var;
    }
  }
  return NULL;
}

/* 语法解析 */

// 复合语句解析
static Node *compoundStmt(Token **rest, Token *tok);
// 语句解析
static Node *stmt(Token **rest, Token *tok);
static Node *exprStmt(Token **rest, Token *tok);
// 表达式解析
static Node *expr(Token **rest, Token *tok);
// 比较解析
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
// 乘除解析
static Node *mul(Token **rest, Token *tok);
// 一元解析 '-','+'，负号，正号
static Node *unary(Token **Rest, Token *Tok);
// 数字解析
static Node *primary(Token **rest, Token *tok);
// 赋值解析
static Node *assign(Token **rest, Token *tok);

/**
 * @brief 语句解析
 * "return" expr ";"
 *  | "if" "(" expr ")" stmt ("else" stmt)?
 *  | "{" compoundStmt
 *  | exprStmt
 * @param  rest
 * @param  tok
 * @return Node*
 */
static Node *stmt(Token **rest, Token *tok) {
  if (equal(tok, "return")) {
    Node *node = newNode(ND_RETURN, tok);
    node->lhs = expr(&tok, tok->next);
    *rest = skip(tok, ";");
    return node;
  }

  // 解析if语句
  if (equal(tok, "if")) {
    Node *node = newNode(ND_IF, tok);
    tok = skip(tok->next, "(");
    node->cond = expr(&tok, tok);
    tok = skip(tok, ")");
    // then 符合条件后的语句
    node->then = stmt(&tok, tok);
    // else
    if (equal(tok, "else")) {
      node->els = stmt(&tok, tok->next);
    }

    *rest = tok;
    return node;
  }

  // 解析for语句
  if (equal(tok, "for")) {
    Node *node = newNode(ND_FOR, tok);
    tok = skip(tok->next, "(");
    node->init = exprStmt(&tok, tok);
    if (!equal(tok, ";")) {
      node->cond = expr(&tok, tok);
    }
    tok = skip(tok, ";");

    if (!equal(tok, ")")) {
      node->inc = expr(&tok, tok);
    }
    tok = skip(tok, ")");

    node->then = stmt(rest, tok);
    return node;
  }

  // while
  if (equal(tok, "while")) {
    Node *node = newNode(ND_FOR, tok);
    tok = skip(tok->next, "(");
    node->cond = expr(&tok, tok);
    tok = skip(tok, ")");
    node->then = stmt(rest, tok);
    return node;
  }

  // "{" compoundStmt
  if (equal(tok, "{")) {
    return compoundStmt(rest, tok->next);
  }

  // exprStmt
  return exprStmt(rest, tok);
}

/**
 * @brief 复合语句解析
 * @param  rest
 * @param  tok
 * @return Node*
 */
static Node *compoundStmt(Token **rest, Token *tok) {
  Node *node = newNode(ND_BLOCK, tok);
  Node head = {};
  Node *cur = &head;

  while (!equal(tok, "}")) {
    cur = cur->next = stmt(&tok, tok);
  }

  node->body = head.next;
  *rest = tok->next;
  return node;
}

/**
 * @brief 表达式语句解析 expr?*";"
 * @param  rest
 * @param  tok
 * @return Node*
 */
static Node *exprStmt(Token **rest, Token *tok) {
  if (equal(tok, ";")) {
    *rest = tok->next;
    return newNode(ND_BLOCK, tok);
  }

  Node *node = newNode(ND_EXPR_STMT, tok);
  node->lhs = expr(&tok, tok);
  *rest = skip(tok, ";");
  return node;
}
/**
 * @brief  表达式解析
 * @param  rest
 * @param  tok
 * @return Node*
 */
static Node *expr(Token **rest, Token *tok) { return assign(rest, tok); }

/**
 * @brief 赋值解析
 * @param  rest
 * @param  tok
 * @return Node*
 */
static Node *assign(Token **rest, Token *tok) {
  Node *node = equality(&tok, tok);
  if (equal(tok, "=")) {
    return node = newBinary(ND_ASSIGN, node, assign(rest, tok->next), tok);
  }
  *rest = tok;
  return node;
}

/**
 * @brief 比较解析
 * @param  rest
 * @param  tok
 * @return Node*
 */
static Node *equality(Token **rest, Token *tok) {
  Node *node = relational(&tok, tok);
  while (true) {
    Token *start = tok;

    // "=="
    if (equal(tok, "==")) {
      node = newBinary(ND_EQ, node, relational(&tok, tok->next), start);
      continue;
    }

    // "!="
    if (equal(tok, "!=")) {
      node = newBinary(ND_NE, node, relational(&tok, tok->next), start);
      continue;
    }

    *rest = tok;
    return node;
  }
}

/**
 * @brief 解析比较关系
 * @param  Rest
 * @param  Tok
 * @return Node*
 */
static Node *relational(Token **rest, Token *tok) {
  Node *node = add(&tok, tok);
  while (true) {
    Token *start = tok;
    // "<"
    if (equal(tok, "<")) {
      node = newBinary(ND_LT, node, add(&tok, tok->next), start);
      continue;
    }

    // "<="
    if (equal(tok, "<=")) {
      node = newBinary(ND_LE, node, add(&tok, tok->next), start);
      continue;
    }

    // ">"
    if (equal(tok, ">")) {
      node = newBinary(ND_LT, add(&tok, tok->next), node, start);
      continue;
    }

    // ">="
    if (equal(tok, ">=")) {
      node = newBinary(ND_LE, add(&tok, tok->next), node, start);
      continue;
    }

    *rest = tok;
    return node;
  }
}

/**
 * @brief
 * @param  rest
 * @param  tok
 * @return Node*
 */
static Node *add(Token **rest, Token *tok) {
  Node *node = mul(&tok, tok);

  while (true) {
    if (equal(tok, "+")) {
      node = newBinary(ND_ADD, node, mul(&tok, tok->next), tok);
      continue;
    }

    if (equal(tok, "-")) {
      node = newBinary(ND_SUB, node, mul(&tok, tok->next), tok);
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
  Node *node = unary(&tok, tok);
  while (true) {
    if (equal(tok, "*")) {
      node = newBinary(ND_MUL, node, unary(&tok, tok->next), tok);
      continue;
    }

    if (equal(tok, "/")) {
      node = newBinary(ND_DIV, node, unary(&tok, tok->next), tok);
      continue;
    }

    *rest = tok;
    return node;
  }
}

/**
 * @brief 解析一元运算
 * @param  rest
 * @param  tok
 * @return Node*
 */
static Node *unary(Token **rest, Token *tok) {
  if (equal(tok, "+")) {
    return unary(rest, tok->next);
  }

  if (equal(tok, "-")) {
    return newUnary(ND_NEG, unary(rest, tok->next), tok);
  }

  return primary(rest, tok);
}

/**
 * @brief  bracket, number, variable
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

  // variable
  if (tok->kind == TK_IDENT) {
    Obj *var = findVar(tok);
    if (!var) {
      var = newLocalVar(strndup(tok->loc, tok->len));
    }
    *rest = tok->next;
    return newVarNode(var, tok);
  }

  // number
  if (tok->kind == TK_NUM) {
    Node *node = newNum(tok->val, tok);
    *rest = tok->next;
    return node;
  }

  errorTok(tok, "expected an expression");
  return NULL;
}

/**
 * @brief 语法解析入口函数
 * @param  tok
 * @return Node*
 */
Function *parse(Token *tok) {

  tok = skip(tok, "{");

  // 函数体存储语句的AST，locals存储局部变量
  Function *prog = calloc(1, sizeof(Function));
  prog->body = compoundStmt(&tok, tok);
  prog->locals = locals;

  return prog;
}