// RUN: %clang_cc1 -fsyntax-only -flifetime-infer-test-mode -flifetime-safety-inference -Rlifetime-safety-escape-inference -Wno-dangling -verify %s

// Intra-procedural escape inference: every case here is decided purely from
// the analyzed function's own body, with no call to another function
// involved.

struct [[gsl::Owner]] MyObj {
  int id;
  ~MyObj() {} // Non-trivial destructor
};

struct [[gsl::Pointer()]] View {
  View(const MyObj &); // Borrows from MyObj
  View();
};

// Explicit lifetimebound is suppressed under test mode, so inference must
// independently conclude 'obj' does not escape -- matching the body, not
// the (now-invisible) ground-truth annotation.
View ignores_wrong_explicit_annotation(const MyObj &obj [[clang::lifetimebound]]) { // expected-remark {{parameter 'obj' is inferred to not escape the function}}
  return View();
}

View escapes_via_return(const MyObj &obj) { // expected-remark {{parameter 'obj' is inferred to escape the function}}
  return obj;
}

void does_not_escape(const MyObj &obj) {} // expected-remark {{parameter 'obj' is inferred to not escape the function}}

struct Box {
  // 'this' itself escapes via return.
  Box &self() { return *this; } // expected-remark {{the implicit 'this' parameter is inferred to escape the function}}

  // 'this' does not escape: only a copy of a member is returned, not a
  // reference/pointer derived from 'this'.
  int getId() const { return id; } // expected-remark {{the implicit 'this' parameter is inferred to not escape the function}}

  int id;
};
