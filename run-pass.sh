#!/usr/bin/env bash

# To use the bundled libc++
export LDFLAGS="-L/usr/local/opt/llvm/lib -lc++abi"

# Rebuild the pass and run it with the homebrew version of clang
cd build \
    && echo "---- Make is running ----" \
    && make \
    && echo "\n---- Pass is running ----" \
    && clang-3.7 -Xclang -load -Xclang ../build/skeleton/libSkeletonPass.* ../TestCode.c
