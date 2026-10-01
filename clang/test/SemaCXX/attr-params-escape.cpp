// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify %s
// expected-no-diagnostics

struct S {
  void m(int *p, int *q) __attribute__((params_escape(2, 3)));
  void self(int *p) __attribute__((params_escape(1)));
  void ref(int &r) __attribute__((params_escape(2)));
  static void s(int *p) __attribute__((params_escape(1)));
  void e(this S &, int *p) __attribute__((params_escape(2)));
};

template <typename T> void t(T p) __attribute__((params_escape(1)));

void use(int *p) { t(p); }
