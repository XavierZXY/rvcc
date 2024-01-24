#include "rvcc.h"

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
 * @brief 新建一个单插树
 * @param  kind
 * @param  expr
 * @return Node*
 */
static Node *newUnary(NodeKind kind, Node *expr) {
  Node *node = newNode(kind);
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

// 语句解析
static Node *stmt(Token **rest, Token *tok);
static Node *exprStmt(Token **rest, Token *tok);
// 加减解析
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

/**
 * @brief 语句解析
 * @param  rest              
 * @param  tok               
 * @return Node* 
 */
static Node *stmt(Token **rest, Token *tok) {
  return exprStmt(rest, tok);
}
static Node *exprStmt(Token **rest, Token *tok) {
  Node *node = newUnary(ND_EXPR_STMT, expr(&tok, tok));
  *rest = skip(tok, ";");
  return node;
}
/**
 * @brief  表达式解析
 * @param  rest
 * @param  tok
 * @return Node*
 */
static Node *expr(Token **rest, Token *tok) { return equality(rest, tok); }

/**
 * @brief 比较解析
 * @param  rest              
 * @param  tok               
 * @return Node* 
 */
static Node *equality(Token **rest, Token *tok) {
  Node *node = relational(&tok, tok);
  while (true) {
    // "=="
    if (equal(tok, "==")) {
      node = newBinary(ND_EQ, node, relational(&tok, tok->next));
      continue;
    }

    // "!="
    if (equal(tok, "!=")) {
      node = newBinary(ND_NE, node, relational(&tok, tok->next));
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
static Node *relational(Token **Rest, Token *Tok) {
  Node *node = add(&Tok, Tok);
  while (true) {
    // "<"
    if (equal(Tok, "<")) {
      node = newBinary(ND_LT, node, add(&Tok, Tok->next));
      continue;
    }

    // "<="
    if (equal(Tok, "<=")) {
      node = newBinary(ND_LE, node, add(&Tok, Tok->next));
      continue;
    }

    // ">"
    if (equal(Tok, ">")) {
      node = newBinary(ND_LT, add(&Tok, Tok->next), node);
      continue;
    }

    // ">="
    if (equal(Tok, ">=")) {
      node = newBinary(ND_LE, add(&Tok, Tok->next), node);
      continue;
    }

    *Rest = Tok;
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
  Node *node = unary(&tok, tok);
  while (true) {
    if (equal(tok, "*")) {
      node = newBinary(ND_MUL, node, unary(&tok, tok->next));
      continue;
    }

    if (equal(tok, "/")) {
      node = newBinary(ND_DIV, node, unary(&tok, tok->next));
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
    return newUnary(ND_NEG, unary(rest, tok->next));
  }

  return primary(rest, tok);
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

/**
 * @brief 语法解析入口函数
 * @param  tok
 * @return Node*
 */
Node *parse(Token *tok) {
  Node head = {};
  Node *cur = &head;

  while (tok->kind != TK_EOF) {
    cur = cur->next = stmt(&tok, tok);
  }
  return head.next;
}