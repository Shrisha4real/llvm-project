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

void in_parallel_for(int n) {
#pragma omp parallel for
  for (int i = 0; i < n; i++) {
#pragma omp atomic
    arr[0] += i;
  }
}

// CIR-LABEL: cir.func {{.*}}@increment()
// CIR:         omp.atomic.update %{{.*}} : !cir.ptr<!s32i> {
// CIR-NEXT:    ^bb0(%[[OLD:.*]]: !s32i{{.*}}):
// CIR-NEXT:      %[[NEW:.*]] = cir.add nsw %[[OLD]], %{{.*}} : !s32i
// CIR-NEXT:      omp.yield(%[[NEW]] : !s32i)
// CIR-NEXT:    } atomic_control <>
// CIR-NEXT:    cir.return

// CIR-LABEL: cir.func {{.*}}@prefix_decrement()
// CIR:         omp.atomic.update %{{.*}} : !cir.ptr<!s32i> {
// CIR-NEXT:    ^bb0(%[[OLD:.*]]: !s32i{{.*}}):
// CIR-NEXT:      %[[NEW:.*]] = cir.sub nsw %[[OLD]], %{{.*}} : !s32i
// CIR-NEXT:      omp.yield(%[[NEW]] : !s32i)

// The operand 'expr' is computed before the atomic operation.
// CIR-LABEL: cir.func {{.*}}@compound(
// CIR:         %[[MUL:.*]] = cir.mul nsw
// CIR-NEXT:    omp.atomic.update %{{.*}} : !cir.ptr<!s32i> {
// CIR-NEXT:    ^bb0(%[[OLD:.*]]: !s32i{{.*}}):
// CIR-NEXT:      %[[NEW:.*]] = cir.add nsw %[[OLD]], %[[MUL]] : !s32i
// CIR-NEXT:      omp.yield(%[[NEW]] : !s32i)

// CIR-LABEL: cir.func {{.*}}@x_binop_expr(
// CIR:         omp.atomic.update %{{.*}} : !cir.ptr<!s32i> {
// CIR-NEXT:    ^bb0(%[[OLD:.*]]: !s32i{{.*}}):
// CIR-NEXT:      %[[NEW:.*]] = cir.sub nsw %[[OLD]], %{{.*}} : !s32i
// CIR-NEXT:      omp.yield(%[[NEW]] : !s32i)

// CIR-LABEL: cir.func {{.*}}@expr_binop_x(
// CIR:         omp.atomic.update %{{.*}} : !cir.ptr<!s32i> {
// CIR-NEXT:    ^bb0(%[[OLD:.*]]: !s32i{{.*}}):
// CIR-NEXT:      %[[NEW:.*]] = cir.sub nsw %{{.*}}, %[[OLD]] : !s32i
// CIR-NEXT:      omp.yield(%[[NEW]] : !s32i)

// CIR-LABEL: cir.func {{.*}}@float_update()
// CIR:         omp.atomic.update %{{.*}} : !cir.ptr<!cir.float> {
// CIR-NEXT:    ^bb0(%[[OLD:.*]]: !cir.float{{.*}}):
// CIR-NEXT:      %[[NEW:.*]] = cir.fmul %[[OLD]], %{{.*}} : !cir.float
// CIR-NEXT:      omp.yield(%[[NEW]] : !cir.float)

// CIR-LABEL: cir.func {{.*}}@double_update(
// CIR:         omp.atomic.update %{{.*}} : !cir.ptr<!cir.double> {
// CIR-NEXT:    ^bb0(%[[OLD:.*]]: !cir.double{{.*}}):
// CIR-NEXT:      %[[NEW:.*]] = cir.fadd %[[OLD]], %{{.*}} : !cir.double
// CIR-NEXT:      omp.yield(%[[NEW]] : !cir.double)

// CIR-LABEL: cir.func {{.*}}@unsigned_update(
// CIR:         omp.atomic.update %{{.*}} : !cir.ptr<!u32i> {
// CIR-NEXT:    ^bb0(%[[OLD:.*]]: !u32i{{.*}}):
// CIR-NEXT:      %[[NEW:.*]] = cir.or %[[OLD]], %{{.*}} : !u32i
// CIR-NEXT:      omp.yield(%[[NEW]] : !u32i)

// CIR-LABEL: cir.func {{.*}}@char_update()
// CIR:         omp.atomic.update %{{.*}} : !cir.ptr<!s8i> {
// CIR-NEXT:    ^bb0(%[[OLD:.*]]: !s8i{{.*}}):
// CIR-NEXT:      %[[WIDE:.*]] = cir.cast integral %[[OLD]] : !s8i -> !s32i
// CIR-NEXT:      %[[SUM:.*]] = cir.add nsw %[[WIDE]], %{{.*}} : !s32i
// CIR-NEXT:      %[[NEW:.*]] = cir.cast integral %[[SUM]] : !s32i -> !s8i
// CIR-NEXT:      omp.yield(%[[NEW]] : !s8i)

// CIR-LABEL: cir.func {{.*}}@through_pointer(
// CIR:         %[[PTR:.*]] = cir.load {{.*}} : !cir.ptr<!cir.ptr<!s32i>>, !cir.ptr<!s32i>
// CIR:         omp.atomic.update %[[PTR]] : !cir.ptr<!s32i> {

// CIR-LABEL: cir.func {{.*}}@array_element(
// CIR:         %[[ELT:.*]] = cir.get_element inbounds
// CIR:         omp.atomic.update %[[ELT]] : !cir.ptr<!s32i> {
// CIR-NEXT:    ^bb0(%[[OLD:.*]]: !s32i{{.*}}):
// CIR-NEXT:      %[[NEW:.*]] = cir.xor %[[OLD]], %{{.*}} : !s32i
// CIR-NEXT:      omp.yield(%[[NEW]] : !s32i)

// CIR-LABEL: cir.func {{.*}}@struct_member()
// CIR:         %[[FIELD:.*]] = cir.get_member %{{.*}}[1] {name = "b"}
// CIR:         omp.atomic.update %[[FIELD]] : !cir.ptr<!s32i> {
// CIR-NEXT:    ^bb0(%[[OLD:.*]]: !s32i{{.*}}):
// CIR-NEXT:      %[[NEW:.*]] = cir.sub nsw %[[OLD]], %{{.*}} : !s32i
// CIR-NEXT:      omp.yield(%[[NEW]] : !s32i)

// CIR-LABEL: cir.func {{.*}}@expr_with_call()
// CIR:         %[[CALL:.*]] = cir.call @side_effect()
// CIR-NEXT:    omp.atomic.update %{{.*}} : !cir.ptr<!s32i> {
// CIR-NEXT:    ^bb0(%[[OLD:.*]]: !s32i{{.*}}):
// CIR-NEXT:      %[[NEW:.*]] = cir.add nsw %[[OLD]], %[[CALL]] : !s32i
// CIR-NEXT:      omp.yield(%[[NEW]] : !s32i)
// CIR-NOT:     cir.call @side_effect

// CIR-LABEL: cir.func {{.*}}@with_hint(
// CIR:         omp.atomic.update hint(nonspeculative) %{{.*}} : !cir.ptr<!s32i> {

// CIR-LABEL: cir.func {{.*}}@explicit_update()
// CIR:         omp.atomic.update %{{.*}} : !cir.ptr<!s32i> {
// CIR:           cir.add nsw
// CIR:           omp.yield

// CIR-LABEL: cir.func {{.*}}@in_parallel()
// CIR:         omp.parallel {
// CIR:           omp.atomic.update %{{.*}} : !cir.ptr<!s32i> {
// CIR:             omp.yield
// CIR:           } atomic_control <>
// CIR-NEXT:      omp.terminator
// CIR-NEXT:    }

// CIR-LABEL: cir.func {{.*}}@in_parallel_for(
// CIR:         omp.parallel {
// CIR:           omp.wsloop {{.*}}{
// CIR:             omp.loop_nest
// CIR:               omp.atomic.update %{{.*}} : !cir.ptr<!s32i> {
// CIR:                 omp.yield(%{{.*}} : !s32i)
// CIR:               } atomic_control <>

// LLVM-LABEL: define {{.*}}void @increment()
// LLVM:         atomicrmw add ptr @counter, i32 1 monotonic, align 4

// LLVM-LABEL: define {{.*}}void @prefix_decrement()
// LLVM:         atomicrmw sub ptr @counter, i32 1 monotonic, align 4

// LLVM-LABEL: define {{.*}}void @compound(
// LLVM:         atomicrmw add ptr @counter, i32 %{{.*}} monotonic, align 4

// LLVM-LABEL: define {{.*}}void @x_binop_expr(
// LLVM:         atomicrmw sub ptr @counter, i32 %{{.*}} monotonic, align 4

// 'v - x' has no atomicrmw form, so it becomes a compare-and-swap loop.
// LLVM-LABEL: define {{.*}}void @expr_binop_x(
// LLVM:         load atomic i32, ptr @counter monotonic, align 4
// LLVM:         cmpxchg ptr @counter, i32 %{{.*}}, i32 %{{.*}} monotonic monotonic, align 4

// LLVM-LABEL: define {{.*}}void @float_update()
// LLVM:         load atomic i32, ptr @fvalue monotonic, align 4
// LLVM:         cmpxchg ptr @fvalue, i32 %{{.*}}, i32 %{{.*}} monotonic monotonic, align 4

// LLVM-LABEL: define {{.*}}void @double_update(
// LLVM:         atomicrmw fadd ptr @dvalue, double %{{.*}} monotonic, align 8

// LLVM-LABEL: define {{.*}}void @unsigned_update(
// LLVM:         atomicrmw or ptr @uvalue, i32 %{{.*}} monotonic, align 4

// ClangIR keeps the casts to int and back, so the update does not match an
// atomicrmw and becomes a compare-and-swap loop. Classic codegen strips them.
// LLVM-LABEL: define {{.*}}void @char_update()
// LLVM-CIR:     load atomic i8, ptr @cvalue monotonic, align 1
// LLVM-CIR:     cmpxchg ptr @cvalue, i8 %{{.*}}, i8 %{{.*}} monotonic monotonic, align 1
// OGCG:         atomicrmw add ptr @cvalue, i8 1 monotonic, align 1

// LLVM-LABEL: define {{.*}}void @through_pointer(
// LLVM:         atomicrmw add ptr %{{.*}}, i32 1 monotonic, align 4

// LLVM-LABEL: define {{.*}}void @array_element(
// LLVM:         atomicrmw xor ptr %{{.*}}, i32 1 monotonic, align 4

// LLVM-LABEL: define {{.*}}void @struct_member()
// LLVM:         atomicrmw sub ptr getelementptr inbounds{{.*}}@s, i64 4), i32 3 monotonic, align 4

// LLVM-LABEL: define {{.*}}void @expr_with_call()
// LLVM:         %[[CALL:.*]] = call i32 @side_effect()
// LLVM:         atomicrmw add ptr @counter, i32 %[[CALL]] monotonic, align 4

// LLVM-LABEL: define {{.*}}void @with_hint(
// LLVM:         cmpxchg ptr @counter, i32 %{{.*}}, i32 %{{.*}} monotonic monotonic, align 4

// LLVM-LABEL: define {{.*}}void @explicit_update()
// LLVM:         atomicrmw add ptr @counter, i32 1 monotonic, align 4

// LLVM-LABEL: define {{.*}}void @in_parallel()
// LLVM:         call {{.*}}@__kmpc_fork_call(
// LLVM-CIR-LABEL: define {{.*}}void @in_parallel..omp_par(
// OGCG-LABEL:     define {{.*}}void @in_parallel.omp_outlined(
// LLVM:         atomicrmw add ptr @counter, i32 1 monotonic, align 4

// LLVM-LABEL: define {{.*}}void @in_parallel_for(
// LLVM:         call {{.*}}@__kmpc_fork_call(
// LLVM-CIR-LABEL: define {{.*}}void @in_parallel_for..omp_par(
// OGCG-LABEL:     define {{.*}}void @in_parallel_for.omp_outlined(
// LLVM:         atomicrmw add ptr @arr, i32 %{{.*}} monotonic, align {{.*}}
