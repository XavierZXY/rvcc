#include "rvcc.h"

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
  switch (node->kind) {
  case ND_NUM:
    printf("  li a0, %d\n", node->val);
    return;
  case ND_NEG:
    genExpr(node->lhs);
    printf("  neg a0, a0\n");
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
 * @brief 代码生成入口函数
 * @param  node              
 */
void codegen(Node *node) {
  // 声明一个全局main段，同时也是程序入口段
  printf(".globl main\n");
  // main段标签
  printf("main:\n");

  genExpr(node);

  // 生成程序结束指令
  printf("  ret\n");

  assert(Depth == 0);
}