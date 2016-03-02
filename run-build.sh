# Set the path to the homebrew LLVM cmake when running cmake and make
mkdir -p build \
    && cd build \
    && LLVM_DIR=/usr/local/lib/llvm-3.7/share/llvm/cmake cmake .. \
    && make
