// RUN: %clang_cc1 -fblocks -fsyntax-only -verify -Wno-objc-root-class %s

@interface I
- (void)m:(int *)p q:(int *)q __attribute__((params_escape(1, 2)));
- (void)bad:(int *)p __attribute__((params_escape(2))); // expected-error {{'params_escape' attribute parameter 1 is out of bounds}}
@end

void f(void) {
  void (^b)(int *) = ^(int *p) __attribute__((params_escape(1))) {};
  void (^c)(int *) __attribute__((params_escape(1))) = b;
}
