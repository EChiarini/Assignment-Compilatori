# Assignment-Compilatori
## Setup
```
export LLVM_DIR=/folder/to/llvm-19/
export ROOT_LABS=/folder/to/Assignment_X/
```
### Struttura Cartelle
```
Assignment_X/
├── build/ 
├── CMakeLists.txt
├── PassFile.cpp
├── test/
│   └── example.ll
│   └── run.sh
```
Dentro alla cartella `build`, eseguire il comando:
```
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR ../
```
Dentro alla cartella `test`, eseguire il comando:
```
make -C ../build/
opt -load-pass-plugin ../build/libLocalOpts.so -p 'nome_passo' esempio.ll -o esempio.bc
llvm-dis esempio.bc -o esempio_dopo_i_passi.ll
cat esempio_dopo_i_passi.ll
```
Per semplicità, per ogni passo ho creato un `.sh` che riassume in un comando i primi tre comandi sopra

## 1° Assignment
### Algebraic Identity
Nome passo: `algebraic-identity` \
Esempio: `ai.ll`
### Strength Reduction
Nome passo: `strength-reduction` \
Esempio: `sr.ll`
### Multi-Instruction Optimization
Nome passo: `multi-opt` \
Esempio: `mo.ll`
### Tutto insieme
Nome passo: `algebraic-identity,strength-reduction,multi-opt` \
Esempio: `all.ll`

## 2° Assignment
