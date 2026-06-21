#ifndef H_GENERATOR_QUALLE
#define H_GENERATOR_QUALLE

#include "parser.h"
#include <stdio.h>

FILE* generate_llvm();
FILE* generate_bitcode(ast *root);

#endif