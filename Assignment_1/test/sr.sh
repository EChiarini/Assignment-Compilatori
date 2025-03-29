make -C ../build/
opt -load-pass-plugin '../build/libStrengthReduction.so' -p strength-reduction sr.ll -o sr.bc
llvm-dis sr.bc -o sr2.ll
