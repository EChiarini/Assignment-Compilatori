Repository del gruppo: Chiarini Emiliano e Siotto Matilde
# 1° Assignment
## Setup
```
export LLVM_DIR=/folder/to/llvm-19/
export ROOT_LABS=/folder/to/Assignment_X/
```
### Struttura Cartelle
```
Assignment_X/
├── build/
├── test/
│   └── example.ll
│   └── run.sh
├── CMakeLists.txt
├── PassFile.cpp
```
Dentro alla cartella `build`, eseguire il comando:
```
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR ../
```
Dentro alla cartella `test`, eseguire i comandi:
```
make -C ../build/
opt -load-pass-plugin ../build/libLocalOpts.so -p 'nome_passo' esempio.ll -o esempio.bc
llvm-dis esempio.bc -o esempio_dopo_i_passi.ll
cat esempio_dopo_i_passi.ll
```
Per semplicità, per ogni passo ho creato un `run.sh` che riassume in un comando i primi tre comandi sopra \
I file che crea sono `(ai/sr/mo)2.ll`

## Algebraic Identity
Nome passo: `algebraic-identity` \
Esempio: `ai.ll`
## Strength Reduction
Nome passo: `strength-reduction` \
Esempio: `sr.ll`
## Multi-Instruction Optimization
Nome passo: `multi-opt` \
Esempio: `mo.ll`

# 2° Assignment
File PDF
# 3° Assignment
Stessa cosa del 1° Assignment
# 4° Assignment
...
