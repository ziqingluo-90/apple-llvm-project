// RUN: %clang_cc1 -fblocks -fsyntax-only -verify %s

void f(int *p) __attribute__((params_escape(1)));
void g(int *p);
typedef void (*cb)(int *) __attribute__((params_escape(1)));
typedef void (*plain)(int *);

void (*ok1)(int *) __attribute__((params_escape(1))) = f;
void (*bad1)(int *) = f; // expected-warning {{'params_escape' attributes of the source and the destination do not match}}
void (*bad2)(int *) __attribute__((params_escape(1))) = g; // expected-warning {{'params_escape' attributes of the source and the destination do not match}}
cb ok2 = f;
plain bad3 = f; // expected-warning {{'params_escape' attributes of the source and the destination do not match}}

struct S { void (*m)(int *) __attribute__((params_escape(1))); };
struct S s1 = {f};
struct S s2 = {g}; // expected-warning {{'params_escape' attributes of the source and the destination do not match}}

void take(cb c);
void call(void) {
  take(f);
  take(g); // expected-warning {{'params_escape' attributes of the source and the destination do not match}}
}

cb ret1(void) { return f; }
cb ret2(void) { return g; } // expected-warning {{'params_escape' attributes of the source and the destination do not match}}

void assign(cb c, plain p) {
  void (*l)(int *) __attribute__((params_escape(1)));
  l = f;
  l = g; // expected-warning {{'params_escape' attributes of the source and the destination do not match}}
  c = g; // expected-warning {{'params_escape' attributes of the source and the destination do not match}}
  p = f; // expected-warning {{'params_escape' attributes of the source and the destination do not match}}
  cb t = 1 ? f : g; // expected-warning {{'params_escape' attributes of the source and the destination do not match}}
}

void blocks(void) {
  void (^b)(int *) __attribute__((params_escape(1))) = ^(int *p) __attribute__((params_escape(1))) {};
  void (^c)(int *) __attribute__((params_escape(1))) = b;
  void (^d)(int *) = b; // expected-warning {{'params_escape' attributes of the source and the destination do not match}}
  void (^e)(int *) __attribute__((params_escape(1))) = ^(int *p) {}; // expected-warning {{'params_escape' attributes of the source and the destination do not match}}
}

void unresolved(void *v) {
  cb c = v;
  cb d = 0;
}
