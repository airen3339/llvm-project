// Tests CUDA compilation targeting sm_next

// CC1 options level check.
// Check that by default we only supply sm_next CPU info without explicitly 
// overriding SM/PTX versions, and letting LLVM pick the defaults.
// RUN: %clang -### -c --target=x86_64-linux-gnu --cuda-device-only  \
// RUN:    --cuda-path=%S/Inputs/CUDA_111/usr/local/cuda %s \
// RUN:    --cuda-gpu-arch=sm_next  2>&1 \
// RUN:   | FileCheck -check-prefixes=ARGS-COMMON,ARGS-ARCH %s
//
// Same, with explicitly set sm and PTX versions.
// RUN: %clang -### -c --target=x86_64-linux-gnu --cuda-device-only \
// RUN:    --cuda-path=%S/Inputs/CUDA_111/usr/local/cuda %s \
// RUN:    --cuda-gpu-arch=sm_next --cuda-next-sm=111 --cuda-next-ptx=222  2>&1 \
// RUN:   | FileCheck -check-prefixes=ARGS-COMMON,ARGS-OVERRIDE %s

// Preprocessor level checks.
// RUN: %clang -dD -E --target=x86_64-linux-gnu --cuda-device-only -nocudainc \
// RUN:    --cuda-path=%S/Inputs/CUDA_111/usr/local/cuda %s \
// RUN:    --cuda-gpu-arch=sm_next  2>&1 \
// RUN:   | FileCheck -check-prefixes=PP-COMMON,PP-ARCH %s
//
// Same, with explicitly set sm and PTX versions.
// RUN: %clang -dD -E --target=x86_64-linux-gnu --cuda-device-only -nocudainc \
// RUN:    --cuda-path=%S/Inputs/CUDA_111/usr/local/cuda %s \
// RUN:    --cuda-gpu-arch=sm_next --cuda-next-sm=111 --cuda-next-ptx=222  2>&1 \
// RUN:   | FileCheck -check-prefixes=PP-COMMON,PP-OVERRIDE %s

// PTX level checks. 
// RUN: %clang -S --target=x86_64-linux-gnu --cuda-device-only -nocudainc -nocudalib \
// RUN:    --cuda-path=%S/Inputs/CUDA_111/usr/local/cuda -o - %s \
// RUN:    --cuda-gpu-arch=sm_next  2>&1 \
// RUN:   | FileCheck -check-prefixes=PTX-ARCH %s
//
// Same, with explicitly set sm and PTX versions.
// RUN: %clang -S --target=x86_64-linux-gnu --cuda-device-only -nocudainc -nocudalib \
// RUN:      --cuda-path=%S/Inputs/CUDA_111/usr/local/cuda -o - %s \
// RUN:      --cuda-gpu-arch=sm_next --cuda-next-sm=111 --cuda-next-ptx=222  2>&1 \
// RUN:   | FileCheck -check-prefixes=PTX-OVERRIDE %s


// ARGS-COMMON: "-cc1" "-triple" "nvptx64-nvidia-cuda"
// We need to pass specific SM version to CC1, so that preprocessor can set __CUDA_ARCH__ macro
// ARGS-ARCH-SAME: "--cuda-next-sm=52" "-mllvm" "--nvptx-next-sm=52"
// .. but we do not explicitly set PTX version, and carry on with the default for the detected CUDA SDK.
// ARGS-ARCH-NOT: --cuda-next-ptx=
// ARGS-ARCH-NOT: --nvptx-next-ptx=
// When we override SM and PTX versions, we explicitly set them for both clang and LLVM.
// ARGS-OVERRIDE-SAME: "--cuda-next-sm=111" "-mllvm" "--nvptx-next-sm=111" "--cuda-next-ptx=222" "-mllvm" "--nvptx-next-ptx=222"
// ARGS-COMMON-SAME: "-target-cpu" "sm_next"
// ARGS-COMMON-SAME: "-target-feature" "+ptx71"
// ARGS-COMMON-NEXT: ptxas
// ARGS-ARCH-SAME: "--gpu-name" "sm_52"
// ARGS-OVERRIDE-SAME: "--gpu-name" "sm_111"
//
//
// PP-COMMON:  #define __NVPTX__ 1
// PP-ARCH: #define __CUDA_ARCH__ 520
// PP-OVERRIDE: #define __CUDA_ARCH__  1110
//
//
// PTX-ARCH:  .version 8.5
// PTX-ARCH:  .target sm_52
// PTX-OVERRIDE:  .version 22.2
// PTX-OVERRIDE:  .target sm_111

