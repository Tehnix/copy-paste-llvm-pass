# To use the bundled libc++
LDFLAGS="-L/usr/local/opt/llvm/lib -lc++abi"

# Jump into the build directory
cd build

# Rebuild the pass and run it with the homebrew version of clang
echo "---- Make is running ----" \
    && make \
    && echo "\n---- Pass is running ----" \
    && clang-3.7 -Xclang -load -Xclang ../build/skeleton/libSkeletonPass.* ../TestCode.c
