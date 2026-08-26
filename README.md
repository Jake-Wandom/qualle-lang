# qualle-lang
QUALLE is a quantum programming language created to use LLVM QIR to compile to quantum hardware.
This is intended to be a proof of concept for my Bachelor Thesis.

QUALLE is an acronym for QUantum Adaptive LLVM Experiment.

This repo contains the QUALLE compiler named quallcom.

## Installation
**Requirements**: GNU Compiler Collection(gcc), LLVM
1. Install required packages
2. Clone this repository: git clone https://github.com/Jake-Wandom/qualle-lang.git
3. Open the directory: cd qualle-lang
4. Compile QUALLE: make clean && make
5. Add the binary to your PATH: echo 'export PATH=$PATH:/PATH/TO/DIRECTORY' >> ~/.bashrc

You can now use the compiler with the command quallcom and given flags and files.
You can try a few of the given examples in this repo.

## Support
QUALLE is QIR base profile compliant and supports the following quantum functions:
- H
- X
- Y
- Z
- RX
- RY
- RZ
- S
- T
- CX
- CY
- CZ
- SWAP
- RXX
- RYY
- RZZ

