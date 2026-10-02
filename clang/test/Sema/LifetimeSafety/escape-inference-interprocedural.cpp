// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -fsyntax-only -flifetime-infer-test-mode -flifetime-safety-inference -fexperimental-lifetime-safety-tu-analysis -Rlifetime-safety-escape-inference -Wno-dangling -I%t -verify=tu %t/test.cpp
// RUN: %clang_cc1 -fsyntax-only -flifetime-infer-test-mode -flifetime-safety-inference -Rlifetime-safety-escape-inference -Wno-dangling -I%t -verify=notu %t/test.cpp


//--- common.h
#ifndef COMMON_H
#define COMMON_H

struct [[gsl::Owner]] MyObj {
  int id;
  ~MyObj() {} // Non-trivial destructor
};

struct [[gsl::Pointer()]] View {
  View(const MyObj &); // Borrows from MyObj
  View();
};

// No annotation at all: ground truth is unknown, inference must determine
// from the body alone, both here and once propagated into 'caller'.
View helper(const MyObj &obj);

#endif

//--- test.cpp
#include "common.h"

View caller(const MyObj &obj) {
  // tu-remark@-1 {{parameter 'obj' is inferred to escape the function}}
  // notu-remark@-2 {{parameter 'obj' is inferred to not escape the function}}
  return helper(obj);
}

View helper(const MyObj &obj) {
  // tu-remark@-1 {{parameter 'obj' is inferred to escape the function}}
  // notu-remark@-2 {{parameter 'obj' is inferred to escape the function}}
  return obj;
}

// 'helper' is a confirmed pass-through, but 'contains_passthrough_result'
// never returns (or otherwise escapes) the value it gets back from it, so
// 'obj' must not be inferred as escaping here -- merely flowing through a
// pass-through call is not itself an escape. This should hold the same way
// regardless of call-graph ordering, since it doesn't depend on
// propagation at all.
View contains_passthrough_result(const MyObj &obj) {
  // tu-remark@-1 {{parameter 'obj' is inferred to not escape the function}}
  // notu-remark@-2 {{parameter 'obj' is inferred to not escape the function}}
  helper(obj);
  return View();
}
