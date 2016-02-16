# llvm-pass-skeleton

A completely useless LLVM pass.

Build:

    $ cd llvm-pass-skeleton
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ cd ..

Run:

    $ clang -Xclang -load -Xclang build/skeleton/libSkeletonPass.* TestCode.c

## Helper scripts

Instead of the above, the build step can be done by running `./run-build.sh` and the pass can be compiled and run with `run-pass.sh`.
