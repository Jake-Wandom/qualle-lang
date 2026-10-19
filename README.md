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
4. Compile QUALLE: make
5. Add the binary to your PATH: echo 'export PATH=$PATH:/PATH/TO/DIRECTORY' >> ~/.bashrc

You can now use the compiler with the command quallcom and given flags and files.  
You can try a few of the given examples in this repo.  

## MAKEFILE guide
If you want to recompile QUALLE use make clean && make.  
For additional debug options use make DEBUG=1.  
make test runs a test script that rapidly compiles all example files.  

## How to use quallcom
quallcom only accepts flags and .ql files as input.  
quallcom -h prints this page:  
Compiling quallcom [FLAGS] [FILE] ... [FILE]  

FLAGS:  
  -h or --help: print this page  
  -p or --print: print information like file content, token lists and abstract syntax tree  
  -a or --adaptive: compile to the adaptive QIR profile  
  -o or --optimise: activate optimisations  
  -d: activate dynamic qubit allocation (only valid in the adaptive profile)  
  -l: generates a readable .ll file instead of bitcode  
