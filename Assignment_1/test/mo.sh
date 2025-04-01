make -C ../build/
opt -load-pass-plugin '../build/libMultiOpt.so' -p multi-opt mo.ll -o mo.bc
llvm-dis mo.bc -o mo2.ll
