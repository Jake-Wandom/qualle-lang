#include "scanner.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void print_man_page(void){
    printf("quallcom does not have a man page yet T_T\n");
}

void zero_buffer(char* buffer, size_t size){
    for(size_t i = 0; i < size; i++){
        buffer[i] = 0;
    }
}

void print_token_list(token* first_token){
    while(first_token != NULL){
        switch(first_token->type){
            case INDICATOR:
                printf("[IND %s]", first_token->value);
                break;
            
            case NUMBER:
                printf("[NUM %s]", first_token->value);
                break;

            case WHITESPACE:
                printf("[WHITE]");
                break;
            
            case END_OF_LINE:
                printf("[EOL %c]\n", *(first_token->value));
                break;

            case DELIMITER:
                printf("[DEL %c]", *(first_token->value));
                break;

            case COMMENT:
                printf("[COM %c]", *(first_token->value));
                break;

            case BRACKET_CLOSE:
                printf("[BC %c]", *(first_token->value));
                break;

            case BRACKET_OPEN:
                printf("[BO %c]", *(first_token->value));
                break;

            case OPERATOR:
                printf("[OP %c]", *(first_token->value));
                break;

            case END:
                printf("\n[END]\n\n");
                break;
            
            case UNKOWN:
                if(first_token->value == NULL) printf("[UN]");
                else printf("[UN %c]", *(first_token->value));
                break;
            
            default:
                fprintf(stderr, "Unkown token type %i", first_token->type);
        }
        first_token = first_token->next_token;
    }
}

void free_token_list(token* first_token){
    while(first_token != NULL){
        free(first_token->value);
        if(first_token->next_token == NULL){
            free(first_token);
            break;
        }
        first_token = first_token->next_token;
        free(first_token->prev_token);
    }
}

int main(int argc, char **argv){
    bool qir = 0;
    bool optimisation = 0;
    bool print = 0;
    char stdlib_path[] = "include/stdlib.ql";

    // first we process all input args: flags and files
    // num_of_files and files contain all file pointers to file names from argv
    int num_of_files = 0;
    FILE **files = NULL;
    for(int i = 1; i < argc; i++){
        // check if argv is a flag
        if(argv[i][0] == '-'){
            if(strcmp(argv[i],"--help") == 0){
                print_man_page();
                return 0;
            }
            switch (argv[i][1]){
                // help -> print man page
                case 'h':
                    print_man_page();
                    return 0;
                // qir -> generate quantum ir instead of bitcode
                case 'q':
                    qir = 1;
                    break;
                // optimise -> use optimisation strategies
                case 'o':
                    optimisation = 1;
                    break;
                case 'p':
                    print = 1;
                    break;
                default:
                    fprintf(stderr, "Unkown flag -%c\n", argv[i][1]);
                    return 1;
            }
        } else {
            // if argv is not a flag we interpret it as a file path
            // we increase num_of_files, realloc and try to open the file and store it in files
            num_of_files++;
            files = realloc(files, num_of_files*sizeof(FILE*));
            *(files+num_of_files-1) = fopen(argv[i], "r+");
            
            if(*(files+num_of_files-1) == NULL){
                fprintf(stderr, "Could not locate or open file %s\n", argv[i]);
            }
        }
    }

    // we compile the standard library and compile it before
    FILE *f = fopen(stdlib_path, "r+");
    if(f == NULL){
        fprintf(stderr, "Could not find or failed to open standard library at %s\n", stdlib_path);
        return 1;
    }

    // determine the maximum buffer size
    // we start with the standard lib file size
    fseek(f, 0, SEEK_END);
    size_t buffer_size = ftell(f) + 1;
    size_t line_buffer_size = 4096;
    rewind(f);

    // now repeat for all other files
    for(int i = 0; i < num_of_files; i++){
        fseek(*(files+i), 0, SEEK_END);
        if(ftell(*(files+i)) > buffer_size){
            buffer_size = ftell(*(files+i));
            buffer_size++;
        }
        rewind(*(files+i));
    }
    if(print) printf("BUFFER SIZE: %lu\n", buffer_size);

    
    // now we process all files into token streams
    char* main_buffer = calloc(buffer_size, sizeof(char));
    char* line_buffer = calloc(line_buffer_size, sizeof(char));
    token* first_token;

    // generate tokens for standard lib
    while(fgets(line_buffer, line_buffer_size, f) != NULL){
        if(print) printf("%s", line_buffer);
        strcat(main_buffer, line_buffer);
    }
    if(print) printf("\n");

    first_token = get_token(main_buffer);
    if(print) print_token_list(first_token);

    // atm for tests, we immediatly free the list
    // in the future we will have to save all lists
    free_token_list(first_token);

    // reset buffer
    zero_buffer(main_buffer, buffer_size);
    zero_buffer(line_buffer, line_buffer_size);


    for(int i = 0; i < num_of_files; i++){
        if(print) printf("Content of %i. file:\n",i+1);

        while(fgets(line_buffer, line_buffer_size, *(files+i)) != NULL){
            if(print )printf("%s", line_buffer);
            strcat(main_buffer, line_buffer);
        }
        if(print) printf("\n");

        first_token = get_token(main_buffer);
        if(print) print_token_list(first_token);

        // atm for tests, we immediatly free the list
        // in the future we will have to save all lists
        free_token_list(first_token);

        // reset buffer
        zero_buffer(main_buffer, buffer_size);
        zero_buffer(line_buffer, line_buffer_size);
    }

    // close all files and free all pointers
    for(int i = 0; i < num_of_files; i++){
        fclose(*(files+i));
    }
    free(files);
    free(main_buffer);
    free(line_buffer);
    return 0;
}