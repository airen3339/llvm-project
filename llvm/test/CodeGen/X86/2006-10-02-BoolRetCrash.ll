; RUN: llc < %s 
; PR933
; REQUIRES: default_triple

define fastcc zeroext i1 @test() {
        ret i1 true
}

