#include "scanner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
tokens are contained in a double linked list
we only have a pointer to the first token and its previous token is always NULL
*/
static token* first_token = NULL;

// creates a new token with unkown type and appends it in the token linked list
token* create_token(token* current_token){
    token* new_token = malloc(sizeof(token));
    new_token->type = UNKOWN;
    new_token->value = NULL;
    new_token->next_token = NULL;

    if(!first_token){
        first_token = new_token;
        first_token->prev_token = NULL;
    } else {
        current_token->next_token = new_token;
        new_token->prev_token = current_token;
    }
    return new_token;
}
/*
check_token() determines depending on the char what token we have
this is done partially, the progress in recognising i.e. a command is saved in the value
*/
token* check_token(char chr, token* tok){
    token* current_token = tok;

    switch(chr) {
        case ' ':
            if(current_token->type != WHITESPACE){
                current_token = create_token(current_token);
                current_token->type = WHITESPACE;
                //*(current_token->value) = ' ';
            }
            break;

        case '\n':
        case ';':
            current_token = create_token(current_token);
            current_token->type = END_OF_LINE;
            current_token->value = malloc(sizeof(char));
            *(current_token->value) = ';';
            break;

        case '\0':
            current_token = create_token(current_token);
            current_token->type = END;
            current_token->value = malloc(sizeof(char));
            *(current_token->value) = '\0';
            break;
        
        case ',':
        case '.':
            current_token = create_token(current_token);
            current_token->type = DELIMITER;
            current_token->value = malloc(sizeof(char));
            *(current_token->value) = chr;
            break;
        
        case '(':
        case '[':
        case '{':
            current_token = create_token(current_token);
            current_token->type = BRACKET_OPEN;
            current_token->value = malloc(sizeof(char));
            *(current_token->value) = chr;
            break;

        case ')':
        case ']':
        case '}':
            current_token = create_token(current_token);
            current_token->type = BRACKET_CLOSE;
            current_token->value = malloc(sizeof(char));
            *(current_token->value) = chr;
            break;

        case '#':
            current_token = create_token(current_token);
            current_token->type = COMMENT;
            current_token->value = malloc(sizeof(char));
            *(current_token->value) = chr;
            break;

        // At the moment it is not possible to use - in variable names this could change in the future
        // It should be noted that 39 is the decimal value for '
        case '+':
        case '-':
        case '*':
        case '/':
        case '^':
        case '%':
        case '"':
        case 39:
        case '<':
        case '>':
        case '=':
        case '&':
        case '|':
        case '!':
        case '?':
        case ':':
            current_token = create_token(current_token);
            current_token->type = OPERATOR;
            current_token->value = malloc(sizeof(char));
            *(current_token->value) = chr;
            break;
        
        case 'a' ... 'z':
        case 'A' ... 'Z':
        case '_':
            if(current_token->type == STRING){
                printf("strlen = %lu\n", strlen(current_token->value));
                printf("value: %s\n", current_token->value);
                if((strlen(current_token->value) % 8) == 0){
                    current_token->value = realloc(current_token->value, strlen(current_token->value)+(8*sizeof(char)));
                }
                char* cat = malloc(sizeof(char));
                *cat = chr;
                strcat(current_token->value, cat);
            } else {
                current_token = create_token(current_token);
                current_token->type = STRING;
                current_token->value = malloc(8*sizeof(char));
                *(current_token->value) = chr;
            }
            break;

        case '0' ... '9':
            if(current_token->type == NUMBER){
                if((strlen(current_token->value) % 8) == 0){
                    current_token->value = realloc(current_token->value, strlen(current_token->value)+(8*sizeof(char)));
                }
                strcat(current_token->value, &chr);
            } else {
                current_token = create_token(current_token);
                current_token->type = NUMBER;
                current_token->value = malloc(8*sizeof(char));
                *(current_token->value) = chr;
            }
            break;
        
        default:
            if(chr){
                current_token = create_token(current_token);
                current_token->type = UNKOWN;
                current_token->value = malloc(sizeof(char));
                *(current_token->value) = chr;
            } else {
                fprintf(stderr, "unable to recognize character %c\n", chr);
            }
    }
    return current_token;
}

token* get_token(char* buffer){
    // checking if the string contains \0 to mark the end. We don't have a length so we do not know, if this is the intended end of string.
    if(!strchr(buffer, '\0')){
        fprintf(stderr, "No String end found\n");
        return NULL;
    }

    token* current_token = create_token(NULL);
    char chr = *buffer;
    while(chr != '\0'){
        current_token = check_token(chr, current_token);
        buffer++;
        chr = *buffer;
    }
    
    if(first_token == NULL){
        fprintf(stderr, "Token list empty\n");
    }
    return first_token;
}