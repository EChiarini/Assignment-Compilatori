clang -Xclang -disable-O0-optnone -S -emit-llvm -O0 $1 -o loopfusion.ll
opt -p mem2reg loopfusion.ll -o lf.bc
llvm-dis lf.bc -o lf.ll

make -C ../build/
opt -load-pass-plugin ../build/libLoopFusion.so -p fake-loop-fusion lf.ll -o lf_ott.bc
llvm-dis lf_ott.bc -o risultato.ll
