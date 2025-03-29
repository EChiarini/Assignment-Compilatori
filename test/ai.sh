make -C ../build/
opt -load-pass-plugin '../build/libAlgebraicIdentity.so' -p algebraic-identity ai.ll -o ai.bc
llvm-dis ai.bc -o ai2.ll
