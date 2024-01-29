#include "rvcc.h"
#include <assert.h>
#include <stdio.h>

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
 * @brief align to
 * @param  N
 * @param  Align
 * @return int
 */
static int alignTo(int n, int align) {
  // (0,Align]返回Align
  return (n + align - 1) / align * align;
}

/**
 * @brief get address of variable
 * @param  node
 */
static void getAddr(Node *node) {
  if (node->kind == ND_VAR) {
    printf("addi a0, fp, -%d\n", node->var->offset);
    return;
  }

  error("not an value");
}

/**
 * @brief genrate expression
 * @param  node
 */
static void genExpr(Node *node) {
  switch (node->kind) {
  case ND_NUM:
    printf("  li a0, %d\n", node->val);
    return;
  case ND_NEG:
    genExpr(node->lhs);
    printf("  neg a0, a0\n");
    return;
  case ND_VAR:
    getAddr(node);
    printf("  ld a0, 0(a0)\n");
    return;
  case ND_ASSIGN:
    getAddr(node->lhs);
    push();
    genExpr(node->rhs);
    pop("a1");
    printf("  sd a0, 0(a1)\n");
    return;
  default:
    break;
  }

  genExpr(node->rhs);
  push();
  genExpr(node->lhs);
  pop("a1");

  /* 生成二叉树结点 */
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
  case ND_EQ:
  case ND_NE:
    // a0=a0^a1，异或指令
    printf("  xor a0, a0, a1\n");

    if (node->kind == ND_EQ)
      // a0==a1
      // a0=a0^a1, sltiu a0, a0, 1
      // 等于0则置1
      printf("  seqz a0, a0\n");
    else
      // a0!=a1
      // a0=a0^a1, sltu a0, x0, a0
      // 不等于0则置1
      printf("  snez a0, a0\n");
    return;
  case ND_LT:
    printf("  slt a0, a0, a1\n");
    return;
  case ND_LE:
    // a0<=a1等价于
    // a0=a1<a0, a0=a1^1
    printf("  slt a0, a1, a0\n");
    printf("  xori a0, a0, 1\n");
    return;
  default:
    break;
  }

  error("invalid expression");
}

/**
 * @brief 生成语句
 * @param  Nd
 */
static void genStmt(Node *node) {
  switch (node->kind) {
  case ND_BLOCK:
    for (Node *n = node->body; n; n = n->next)
      genStmt(n);
    return;
  case ND_RETURN:
    genExpr(node->lhs);
    printf(" j .L.return\n");
    return;
  case ND_EXPR_STMT:
    genExpr(node->lhs);
    return;
  default:
    break;
  }

  error("invalid statement");
}

static void assignLocalVarOffset(Function *prog) {
  int offset = 0;
  for (Obj *var = prog->locals; var; var = var->next) {
    // allocate 8 bytes for each local variable
    offset += 8;
    // set offset from fp
    var->offset = offset;
  }

  prog->stack_size = alignTo(offset, 16);
}

/**
 * @brief 代码生成入口函数
 * @param  node
 */
void codegen(Function *prog) {
  assignLocalVarOffset(prog);

  // 声明一个全局main段，同时也是程序入口段
  printf(".globl main\n");
  // main段标签
  printf("main:\n");

  // 栈布局
  //-------------------------------// sp
  //              fp                  fp = sp-8
  //-------------------------------// fp
  //              'a'                 fp-8
  //              'b'                 fp-16
  //              ...
  //              'z'                 fp-208
  //-------------------------------// sp=sp-8-208
  //           表达式计算
  //-------------------------------//

  /* Prologue, 前言 */
  // 将fp压入栈中，保存fp的值
  printf("  addi sp, sp, -8\n");
  printf("  sd fp, 0(sp)\n");
  // 将sp写入fp
  printf("  mv fp, sp\n");

  // 26个字母*8字节=208字节，栈腾出208字节的空间
  printf("  addi sp, sp, -208\n");

  for (Node *n = prog->body; n; n = n->next) {
    genStmt(n);
    assert(Depth == 0);
  }

  /* Epilogue，后语 */

  // 输出return段标签
  printf(".L.return:\n");
  // 将fp的值改写回sp
  printf("  mv sp, fp\n");
  // 将最早fp保存的值弹栈，恢复fp。
  printf("  ld fp, 0(sp)\n");
  printf("  addi sp, sp, 8\n");

  // 生成程序结束指令
  printf("  ret\n");
}