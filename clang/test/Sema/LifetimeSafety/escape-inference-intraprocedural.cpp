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

MyObj *g_ptr;

// Escapes via assignment to global storage, not return or a ctor field.
// [[clang::lifetimebound]] cannot express this relationship, so this escape
// channel must be detected independently of lifetimebound suggestion/
// inference.
void stores_in_global(MyObj *p) { // expected-remark {{parameter 'p' is inferred to escape the function}}
  g_ptr = p;
}

struct PointerBox {
  // Escapes via assignment to a field outside of a constructor.
  void setPtr(MyObj *p) { ptr = p; } // expected-remark {{parameter 'p' is inferred to escape the function}}
                                      // expected-remark@-1 {{the implicit 'this' parameter is inferred to not escape the function}}
  MyObj *ptr;
};

// Escapes via assignment to a reference out-parameter, with no annotation
// anywhere -- discovered purely from the dataflow, since 'out' aliases
// storage the caller already owns.
void stores_in_out_param(MyObj *p, MyObj *&out) { // expected-remark {{parameter 'p' is inferred to escape the function}}
                                                   // expected-remark@-1 {{parameter 'out' is inferred to not escape the function}}
  out = p;
}

// A reference parameter that is never written must not be reported as
// escaping merely by holding its own placeholder loan.
void out_param_never_written(MyObj *p, MyObj *&out) { // expected-remark {{parameter 'p' is inferred to not escape the function}}
                                                       // expected-remark@-1 {{parameter 'out' is inferred to not escape the function}}
}

// A by-value pointer parameter is not a valid out-parameter sink: reassigning
// it only rebinds a local copy, invisible to the caller.
void stores_in_byvalue_param(MyObj *p, MyObj *out) { // expected-remark {{parameter 'p' is inferred to not escape the function}}
                                                      // expected-remark@-1 {{parameter 'out' is inferred to not escape the function}}
  out = p;
}

// 'out1' is never written; the search for an overlapping out-parameter must
// still find 'out2' and not stop at the first candidate.
void stores_in_second_out_param(MyObj *p, MyObj *&out1, MyObj *&out2) { // expected-remark {{parameter 'p' is inferred to escape the function}}
                                                                         // expected-remark@-1 {{parameter 'out1' is inferred to not escape the function}}
                                                                         // expected-remark@-2 {{parameter 'out2' is inferred to not escape the function}}
  out2 = p;
}

// 'out' captures 'p' but is overwritten before the function returns; only
// the last write to 'out' should determine whether 'p' escapes.
void overwrites_out_param(MyObj *p, MyObj *&out) { // expected-remark {{parameter 'p' is inferred to not escape the function}}
                                                     // expected-remark@-1 {{parameter 'out' is inferred to not escape the function}}
  out = p;
  out = nullptr;
}

struct ThisBox {
  // 'this' escapes via a reference out-parameter, not return or a field.
  void captureThis(ThisBox *&out) { out = this; } // expected-remark {{the implicit 'this' parameter is inferred to escape the function}}
                                                   // expected-remark@-1 {{parameter 'out' is inferred to not escape the function}}
};
