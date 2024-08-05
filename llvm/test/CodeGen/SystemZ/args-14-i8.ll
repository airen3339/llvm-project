; RUN: not --crash llc < %s -mtriple=s390x-linux-gnu 2>&1 | FileCheck %s
; REQUIRES: asserts
;
; Test detection of missing extension of an i8 return value.

define i8 @callee_MissingRetAttr() {
  ret i8 -1
}

; CHECK: Narrow integer argument must have a valid extension type.
