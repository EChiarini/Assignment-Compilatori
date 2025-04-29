clang -Xclang -disable-O0-optnone -S -emit-llvm -O0 verifyLICM.c -o verifyLICM.ll
opt -p mem2reg verifyLICM.ll -o loop.bc
llvm-dis loop.bc -o loop.ll

make -C ../build/
opt -load-pass-plugin ../build/libFakeLICM.so -p fake-licm loop.ll -o loop.bc
llvm-dis loop.bc -o loopLICM.ll
