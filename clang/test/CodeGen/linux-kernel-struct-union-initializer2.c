// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=gnu11 -verify -emit-llvm %s -o - | FileCheck %s
// expected-no-diagnostics

union U1 {
  int x;
  char y[5];
};

struct S1 {
  int x;
  long long y;
};

struct S2 {
  unsigned char b1 : 3; // 1st 3 bits (in 1st byte) are b1
  unsigned char    : 2; // next 2 bits (in 1st byte) are blocked out as unused
  unsigned char b2 : 6; // 6 bits for b2 - doesn't fit into the 1st byte => starts a 2nd
  unsigned char b3 : 2; // 2 bits for b3 - next (and final) bits in the 2nd byte
  int i;
};

struct S3 {
  int x;
} __attribute__((__aligned__(8)));

struct S4 {
  int a;
  union U1 b;
};

// Test non-const initializer for union with padding.
void test1(int x) {
  union U1 a = {x};
  // CHECK-LABEL: @test1
  // CHECK: %[[A:.+]] = alloca %union.U1
  // CHECK: %[[B:.+]] = getelementptr i8, ptr %[[A]], i64 4
  // CHECK-NEXT: call void @llvm.memset.p0.i64(ptr align 4 %[[B]], i8 0, i64 4, i1 false)
}

// Test non-const initializer for struct with padding.
void test2(long long y) {
  struct S1 s = {.y = y};
  // CHECK-LABEL: @test2
  // CHECK: %[[A:.+]] = alloca %struct.S1
  // CHECK: %[[B:.+]] = getelementptr i8, ptr %[[A]], i64 4
  // CHECK-NEXT: call void @llvm.memset.p0.i64(ptr align 4 %[[B]], i8 0, i64 4, i1 false)
}

// Test non-const initializer for struct with padding and bit fields.
void test3(unsigned char b) {
  struct S2 s = {.b1 = b};
  // CHECK-LABEL: @test3
  // CHECK: %[[A:.+]] = alloca %struct.S2
  // CHECK: %[[B:.+]] = getelementptr i8, ptr %[[A]], i64 2
  // CHECK-NEXT: call void @llvm.memset.p0.i64(ptr align 2 %[[B]], i8 0, i64 2, i1 false)
}

// Test non-const initializer for struct with padding at the end of the struct.
void test4(int x) {
  struct S3 s = {x};
  // CHECK-LABEL: @test4
  // CHECK: %[[A:.+]] = alloca %struct.S3
  // CHECK: %[[B:.+]] = getelementptr i8, ptr %[[A]], i64 4
  // CHECK-NEXT: call void @llvm.memset.p0.i64(ptr align 4 %[[B]], i8 0, i64 4, i1 false)
}

// Test non-const initializer for union in struct.
void test5(int a, int b) {
  struct S4 s = {a, {b}};
  // CHECK-LABEL: @test5
  // CHECK: %[[A:.+]] = alloca %struct.S4
  // CHECK: %[[B:.+]] = getelementptr inbounds %struct.S4, ptr %[[A]], i32 0, i32 1
  // CHECK: %[[C:.+]] = getelementptr i8, ptr %[[B]], i64 4
  // CHECK-NEXT: call void @llvm.memset.p0.i64(ptr align 4 %[[C]], i8 0, i64 4, i1 false)
}
