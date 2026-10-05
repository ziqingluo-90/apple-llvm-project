// RUN: %clang_cc1 -fsyntax-only -flifetime-infer-oracle-mode -Rlifetime-safety-escape-inference -verify %s

// With -flifetime-infer-oracle-mode, each source-written noescape emits the
// "not escape" escape inference remark, and inference emits no remarks.

void f(int *p [[clang::noescape]], // expected-remark {{parameter 'p' is inferred to not escape the function}}
       int *q) {}

int *g(int *p) { return p; }
