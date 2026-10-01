// RUN: %clang_cc1 -fsyntax-only -verify %s

void f(int *p, int *q) __attribute__((params_escape(1, 2)));
void dup(int *p) __attribute__((params_escape(1, 1)));
void none(int *p) __attribute__((params_escape)); // expected-error {{'params_escape' attribute takes at least 1 argument}}
void zero(int *p) __attribute__((params_escape(0))); // expected-error {{'params_escape' attribute parameter 1 is out of bounds}}
void big(int *p) __attribute__((params_escape(2))); // expected-error {{'params_escape' attribute parameter 1 is out of bounds}}
void str(int *p) __attribute__((params_escape("a"))); // expected-error {{'params_escape' attribute requires parameter 1 to be an integer constant}}
void val(int n) __attribute__((params_escape(1))); // expected-warning {{'params_escape' attribute only applies to pointer arguments}}

void (*fp)(int *) __attribute__((params_escape(1)));
typedef void (*cb)(int *) __attribute__((params_escape(1)));
struct S { void (*m)(int *) __attribute__((params_escape(1))); };
void g(void (*h)(int *) __attribute__((params_escape(1))));
int v __attribute__((params_escape(1))); // expected-warning {{'params_escape' attribute only applies to functions, methods, blocks, and function pointers}}

void r(int *p, int *q);
void r(int *p, int *q) __attribute__((params_escape(1)));
void r(int *p, int *q) __attribute__((params_escape(2)));
