// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -fopenmp -fclangir -emit-cir %s -o %t.cir
// RUN: FileCheck --input-file=%t.cir %s --check-prefix=CIR
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -fopenmp -fclangir -emit-llvm %s -o %t-cir.ll
// RUN: FileCheck --input-file=%t-cir.ll %s --check-prefixes=LLVM-CIR,LLVM
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -fopenmp -emit-llvm %s -o %t.ll
// RUN: FileCheck --input-file=%t.ll %s --check-prefixes=OGCG,LLVM

// Only 'atomic update', which is the default when no clause is given, is
// implemented. Other atomic kinds are in not-yet-implemented.c.

int counter;
float fvalue;
double dvalue;
unsigned uvalue;
char cvalue;
int arr[4];
struct S { int a; int b; } s;

int side_effect(void);

void increment(void) {
#pragma omp atomic
  counter++;
}

void prefix_decrement(void) {
#pragma omp atomic
  --counter;
}

void compound(int v) {
#pragma omp atomic
  counter += v * 2;
}

void x_binop_expr(int v) {
#pragma omp atomic
  counter = counter - v;
}

// 'x' is the right operand, so the operands of the update are reversed.
void expr_binop_x(int v) {
#pragma omp atomic
  counter = v - counter;
}

void float_update(void) {
#pragma omp atomic
  fvalue *= 2.0f;
}

void double_update(double v) {
#pragma omp atomic
  dvalue += v;
}

void unsigned_update(unsigned v) {
#pragma omp atomic
  uvalue |= v;
}

// 'x' is converted to int and back to char around the update.
void char_update(void) {
#pragma omp atomic
  cvalue += 1;
}

void through_pointer(int *p) {
#pragma omp atomic
  *p += 1;
}

void array_element(int i) {
#pragma omp atomic
  arr[i] ^= 1;
}

void struct_member(void) {
#pragma omp atomic
  s.b -= 3;
}

// 'expr' is evaluated once, before the atomic operation.
void expr_with_call(void) {
#pragma omp atomic
  counter += side_effect();
}

void with_hint(int v) {
#pragma omp atomic hint(4)
  counter = v - counter;
}

void explicit_update(void) {
#pragma omp atomic update
  counter++;
}

void in_parallel(void) {
#pragma omp parallel
  {
#pragma omp atomic
    counter++;
  }
}
