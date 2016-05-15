# Copy/Paste detector

LLVM pass to detect copy/paste code - or in other words, redundant code.

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

## Detection flags

Depending on how aggressive the detector should be, the following flags can be enabled and disabled.

__Implemented:__

* `DetectExtractionToConstant` works in a global scope and finds code that is duplicate and could be extracted into a constant
* `DetectLocalIfThenElse` works locally in the function and finds code that exists both in the if and else blocks of the same if-then-else statement

__Not implemented yet:__

* `DetectGlobalIfThenElse` works globally and finds if-then-else statements that are identical
* `DetectGlobalSimilarAssignments` works globally and finds code blocks that are close to being identical such as
    * `a1 = 5 * value1`
    * `a2 = 1 * value1`
    * `a3 = 3 * value1`
* `DetectGlobalSimilarCalls` works globally detecting similar method calls that vary slightly, such as
    * `method(x*x, x+x, x)`
    * `method(y*y, y+y, x)`
