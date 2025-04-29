make -C ../build/
opt -load-pass-plugin ../build/libFakeLICM.so -p fake-licm loop.ll -o loop.bc
llvm-dis loop.bc -o loopLICM.ll
