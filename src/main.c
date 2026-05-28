#include "scanner.h"
//#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_man_page(){
    printf("qcc does not have a man page yet T_T\n");
}

void zero_buffer(char* buffer, size_t size){
    for(size_t i = 0; i < size; i++){
        buffer[i] = 0;
    }
}

void print_token_list(token* first_token){
    while(first_token != NULL){
        if((first_token->type == STRING) || (first_token->type == NUMBER)){
            printf("Token Type: %i, Token value %s\n", first_token->type, first_token->value);
        } else if (first_token->type != UNKOWN){
            printf("Token Type: %i, Token value %c\n", first_token->type, *(first_token->value));
        } else {
            printf("Token Type: %i, Token value %s\n", first_token->type, first_token->value);
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
                continue;
            }
            switch (argv[i][1]){
                // help -> print man page
                case 'h':
                    print_man_page();
                    return 0;
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
                fprintf(stderr, "Could not open file %s\n", argv[i]);
            }
        }
    }
    
    // now we process all files into token streams
    char* main_buffer = calloc(1024,sizeof(char));
    char* buf = calloc(256, sizeof(char));
    token* first_token;

    for(int i = 0; i < num_of_files; i++){
        printf("Content of %i. file:\n",i+1);
        while(fgets(buf, 256, *(files+i)) != NULL){
            printf("%s", buf);
            strcat(main_buffer, buf);
        }
        printf("\n");

        first_token = get_token(main_buffer);
        print_token_list(first_token);
        // atm for tests, we immediatly free the list
        // in the future we will have to save all lists
        free_token_list(first_token);

        // reset buffer
        zero_buffer(main_buffer, 1024);
        zero_buffer(buf, 256);
    }

    // close all files and free all pointers
    for(int i = 0; i < num_of_files; i++){
        fclose(*(files+i));
    }
    free(files);
    free(main_buffer);
    free(buf);
    return 0;
}