// RUN: %clang_cc1 -fsyntax-only -verify -Wno-objc-root-class %s

@interface A
- (void)m:(int *)p __attribute__((params_escape(1)));
@end

@interface B : A
- (void)m:(int *)p; // expected-warning {{'params_escape' attributes of the source and the destination do not match}} expected-note@4 {{method declared here}}
@end
