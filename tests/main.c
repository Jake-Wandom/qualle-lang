#include "scanner.h"
//#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

void print_token_list(token* first_token){
    while(first_token != NULL){
        printf("Token Type: %i, Token value %s\n", first_token->type, first_token->value);
        first_token = first_token->next_token;
    }
}

int main(){
    FILE* f = fopen("test.ql", "r+");
    if(!f){
        fprintf(stderr, "Could not open file\n");
        return 0;
    }
    char* buf = malloc(sizeof(char)*256);
    fgets(buf, 256, f);
    printf("%s\n", buf);
    fclose(f);

    print_token_list(get_token(buf));
    return 1;
}