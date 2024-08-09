// RUN: set +o pipefail; clang -emit-llvm -c %s -g -o %t
// RUN: llc %t 2>&1 | FileCheck %s
void bad_asm() {
  asm volatile ("BAD SYNTAX$%"); // CHECK: inline-asm-debuginfo.c:4:16: error: unknown token in expression
}

void bad_multi_asm() {
  asm ( ";"
        "BAD SYNTAX;"   // CHECK: inline-asm-debuginfo.c:8:5: error: invalid instruction mnemonic 'bad'
        ";" );
}

void bad_multi_asm_linechg() {
  asm ( ";\n"
        "BAD SYNTAX;\n" // CHECK: inline-asm-debuginfo.c:15:3: error: invalid instruction mnemonic 'bad'
        ";\n" );
}
