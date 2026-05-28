#include "scanner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
tokens are contained in a double linked list
we only have a pointer to the first token and its previous token is always NULL
*/
static token* first_token = NULL;


/*
creates a new token with unkown type and appends it in the token linked list
if the list is not empty and there is not a token provided, the function will not create a new token
instead it will return NULL
*/
token* create_token(token* current_token){
    token* new_token = malloc(sizeof(token));
    new_token->type = UNKOWN;
    new_token->value = NULL;
    new_token->next_token = NULL;

    if(!first_token){
        first_token = new_token;
        first_token->prev_token = NULL;
    } else if(current_token){
        current_token->next_token = new_token;
        new_token->prev_token = current_token;
    } else {
        fprintf(stderr, "Could not append new token to the list\n");
        free(new_token);
        return NULL;
    }
    return new_token;
}
/*
determines depending on the char what token we have
this is basically a huge switch case that assigns chars to tokens
more details are provided in the comments for the unique cases
the function returns either the old or a new token
*/
token* check_token(char chr, token* tok){
    token* current_token = tok;

    switch(chr) {
        // multiple consecutive whitespaces are collapsed into one token since we don't check indentation
        case ' ':
            if(current_token->type != WHITESPACE){
                current_token = create_token(current_token);
                current_token->type = WHITESPACE;
                current_token->value = malloc(sizeof(char));
                *(current_token->value) = ' ';
            }
            break;
        
        // \n and ; are recongised as line breaks and are also collapsed into one token if consecutive
        case '\n':
        case ';':
            if(current_token->type != END_OF_LINE){
                current_token = create_token(current_token);
                current_token->type = END_OF_LINE;
                current_token->value = malloc(sizeof(char));
                *(current_token->value) = ';';
            }
            break;
        
        // normally check_token should not receive \0, this is just a backup
        case '\0':
            current_token = create_token(current_token);
            current_token->type = END;
            current_token->value = malloc(sizeof(char));
            *(current_token->value) = '\0';
            break;
        
        // these delimiters carry different meanings but share the same token for convenience
        case ',':
        case '.':
            current_token = create_token(current_token);
            current_token->type = DELIMITER;
            current_token->value = malloc(sizeof(char));
            *(current_token->value) = chr;
            break;
        
        // for now we just differentiate between open and close brackets
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
        
        // comments are done with a # but I consider also allowing C comments
        case '#':
            current_token = create_token(current_token);
            current_token->type = COMMENT;
            current_token->value = malloc(sizeof(char));
            *(current_token->value) = chr;
            break;
        
        // these operators are mathematical, logical and other
        // at the moment it is not possible to use - in variable names this could change in the future
        // it should be noted that 39 is the decimal value for '
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
        
        // currently only letters and _ can be used for variable names and definitions, this could change in the future
        // we want to avoid a lot of reallocation, thats why we allocate in blocks of 8
        // the ... range syntax is not supported in all C versions
        // it is supported in gcc and clang so this will only change when it becomes a problem
        case 'a' ... 'z':
        case 'A' ... 'Z':
        case '_':
            if(current_token->type == STRING){
                size_t len = strlen(current_token->value);
                if((len % 7) == 0){
                    current_token->value = realloc(current_token->value, len+(8*sizeof(char)));
                }
                *(current_token->value+len) = chr;
                *(current_token->value+len+1) = '\0';
            } else {
                current_token = create_token(current_token);
                current_token->type = STRING;
                current_token->value = calloc(8, sizeof(char));
                *(current_token->value) = chr;
                *(current_token->value+1) = '\0';
            }
            break;
        
        // the scanning for numbers is the same as with letters
        // they are stored as strings, so the parser has to deal with converting them to real numbers
        case '0' ... '9':
            if(current_token->type == NUMBER){
                size_t len = strlen(current_token->value);
                if((len % 8) == 0){
                    current_token->value = realloc(current_token->value, len+(9*sizeof(char)));
                }
                *(current_token->value+len) = chr;
                *(current_token->value+len+1) = '\0';
            } else {
                current_token = create_token(current_token);
                current_token->type = NUMBER;
                current_token->value = calloc(9, sizeof(char));
                *(current_token->value) = chr;
                *(current_token->value+1) = '\0';
            }
            break;
        
        // any unkown characters like start characters will be assigned unkown
        default:
            if(chr){
                current_token = create_token(current_token);
                current_token->type = UNKOWN;
                current_token->value = malloc(sizeof(char));
                *(current_token->value) = chr;
                fprintf(stderr, "unable to recognize character %c,%i\n", chr, chr);
            }
    }
    return current_token;
}

/*
generates a linked list of tokens from the given buffer
returns the first token of the list
*/
token* get_token(char* buffer){
    // make sure we are starting with a fresh token list
    // the main function has to keep track of old lists and free them
    first_token = NULL;
    
    // checking if the string contains \0 to mark the end. We don't have a length so we do not know, if this is the intended end of string.
    if(!strchr(buffer, '\0')){
        fprintf(stderr, "No String end found\n");
        return NULL;
    }

    // we generate the first token in the linked list
    // if first_token already exists it return NULL
    token* current_token = create_token(NULL);
    if(current_token == NULL){
        fprintf(stderr, "Something went wrong, while creating the first token\n");
    }
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