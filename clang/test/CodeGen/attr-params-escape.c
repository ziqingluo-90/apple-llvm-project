// RUN: %clang_cc1 -emit-llvm -o - %s | FileCheck %s

// params_escape must not affect codegen: the annotated declaration gets the
// same IR attributes as the unannotated one.

void a(int *p);
void b(int *p) __attribute__((params_escape(1)));

void use(int *p) {
  a(p);
  b(p);
}

// CHECK: declare void @a(ptr noundef) [[ATTR:#[0-9]+]]
// CHECK: declare void @b(ptr noundef) [[ATTR]]
