#ifndef H_ANALYSER_QUALLE
#define H_ANALYSER_QUALLE

#include "parser.h"

typedef struct {
    enum variable_type type;
    char *name;
    int value;
} variable;

void analyse_ast(ast *root);

#endif