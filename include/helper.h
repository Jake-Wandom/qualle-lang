#ifndef H_HELPER_QUALLE
#define H_HELPER_QUALLE

#include "scanner.h"
#include "parser.h"
#include <stdint.h>
#include <stdio.h>

void print_man_page(void);
void zero_buffer(char* buffer, size_t size);
void print_ast(ast *root, int level);
void print_token_list(token* first_token);
void free_token_list(token* first_token);
void free_ast(ast *root);


#endif