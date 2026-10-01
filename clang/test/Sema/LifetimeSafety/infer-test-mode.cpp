// RUN: %clang_cc1 -ast-dump -ast-dump-filter suppressed_params %s | FileCheck --check-prefix=PRESENT-PARAMS %s
// RUN: %clang_cc1 -ast-dump -ast-dump-filter suppressed_params -flifetime-infer-test-mode %s | FileCheck --check-prefix=ABSENT-PARAMS %s
// RUN: %clang_cc1 -ast-dump -ast-dump-filter SuppressedThis %s | FileCheck --check-prefix=PRESENT-THIS %s
// RUN: %clang_cc1 -ast-dump -ast-dump-filter SuppressedThis -flifetime-infer-test-mode %s | FileCheck --check-prefix=ABSENT-THIS %s

// With -flifetime-infer-test-mode, source-written lifetimebound,
// lifetime_capture_by and noescape attributes are never attached to the AST.

struct MyObj {};
struct [[gsl::Pointer()]] View {};

View suppressed_params(const MyObj &lb [[clang::lifetimebound]],
                       const MyObj *ne [[clang::noescape]],
                       View &res, View cb [[clang::lifetime_capture_by(res)]]);

// PRESENT-PARAMS: ParmVarDecl{{.*}} lb 'const MyObj &'
// PRESENT-PARAMS-NEXT: LifetimeBoundAttr
// PRESENT-PARAMS: ParmVarDecl{{.*}} ne 'const MyObj *'
// PRESENT-PARAMS-NEXT: NoEscapeAttr
// PRESENT-PARAMS: ParmVarDecl{{.*}} cb 'View'
// PRESENT-PARAMS-NEXT: LifetimeCaptureByAttr

// ABSENT-PARAMS: ParmVarDecl{{.*}} lb 'const MyObj &'
// ABSENT-PARAMS-NOT: LifetimeBoundAttr
// ABSENT-PARAMS: ParmVarDecl{{.*}} ne 'const MyObj *'
// ABSENT-PARAMS-NOT: NoEscapeAttr
// ABSENT-PARAMS: ParmVarDecl{{.*}} cb 'View'
// ABSENT-PARAMS-NOT: LifetimeCaptureByAttr

struct SuppressedThis {
  View getLB() const [[clang::lifetimebound]];
  void addCB(View &x) [[clang::lifetime_capture_by(x)]];
};

// PRESENT-THIS: getLB 'View () const{{.*}}lifetimebound
// PRESENT-THIS: addCB 'void (View &){{.*}}lifetime_capture_by(x)

// ABSENT-THIS: getLB 'View () const'
// ABSENT-THIS-NOT: lifetimebound
// ABSENT-THIS: addCB 'void (View &)'
// ABSENT-THIS-NOT: lifetime_capture_by
