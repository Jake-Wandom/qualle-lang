#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static token *current_token;
static ast *last_eol;
static bool in_assign = 0;

/*
creates a new node with all pointer values set to NULL
contrary to create_token, this function does not automatically append
*/
ast* create_node(){
    ast *new_node = calloc(1, sizeof(ast));
    new_node->type = ROOT;
    new_node->branch = NULL;
    new_node->left = NULL;
    new_node->right = NULL;
    new_node->name = NULL;
    new_node->llvm =  NULL;
    new_node->line = current_token->line;

    return new_node;
}

/*
Traverses the token list for num tokens
If the token is NULL we exit, this should not happen
If the token is END, we return it
*/
void switch_token(int num){
    for(int i = 0; i < num; i++){
        if(!current_token){
            fprintf(stderr, "Null pointer while token switch\n");
            exit(1);
        }
        if(current_token->type == END){
            return;
        }
        current_token = current_token->next_token;
    }
}

void handle_error(token *error_token, enum error_type type, char *error_message){

    switch (type){
        case UNEXPECTED_ERROR:
            printf("UNEXPECTED ERROR: %s\n", error_message);
            break;
        case WRONG_TYPE_ERROR:
            printf("WRONG TYPE ERROR: %s\n", error_message);
            break;
        case UNKOWN_SYMBOL_ERROR:
            printf("UNKOWN SYMBOL ERROR: %s\n", error_message);
            break;
        case NO_CONTEXT_ERROR:
            printf("NO CONTEXT ERROR: %s\n", error_message);
            break;
        case UNKOWN_TYPE_ERROR:
            printf("MISSING VARIABLE ERROR: %s\n", error_message);
            break;
        default:
            printf("ERROR: %s\n", error_message);
    }
    // locate position
    printf("line %i ", error_token->line);
    if((error_token->type == INDICATOR) || (error_token->type == NUMBER)){
        printf("->%s<-\n\n", error_token->value);
    } else if((error_token->type != START) && (error_token->type != END)){
        printf("->%c<-\n\n", *(error_token->value));
    }
}

ast* parse_function(ast *current_node){
    ast *new_node = create_node();
    new_node->type = FUNCTION;
    current_node->branch = new_node;

    switch_token(1);

    if(current_token->type != INDICATOR){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected function name");
        return NULL;
    }

    // store function name any double definitions are handled later
    size_t size = strlen(current_token->value)+1;
    new_node->name = malloc(size);
    strncpy(new_node->name, current_token->value, size);


    switch_token(1);

    if((current_token->type != BRACKET_OPEN) || (*(current_token->value) != '(')){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected '(' in function definition");
        return NULL;
    }

    switch_token(1);

    // create subtree with all variables
    ast *temp_node = create_node();
    
    parse_start(temp_node);
    
    new_node->left = temp_node->branch;

    // we need to forward to the end of the function definition
    while(current_token != NULL){
        if((current_token->type == BRACKET_CLOSE) && (*(current_token->value) == ')')){
            break;
        }
        switch_token(1);
    }   

    if((current_token->type != BRACKET_CLOSE) || (*(current_token->value) != ')')){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected ')' in function definition");
        return NULL;
    }

    switch_token(1);

    if((current_token->type != BRACKET_OPEN) || (*(current_token->value) != '{')){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected '{' in function definition");
        return NULL;
    }

    switch_token(1);

    temp_node->branch = NULL;
    temp_node->value = NULL;
    
    parse_start(temp_node);
    
    new_node->right = temp_node->branch;
    free(temp_node);

    // we need to forward to the end of the function body
    while(current_token != NULL){
        if((current_token->type == BRACKET_CLOSE) && (*(current_token->value) == '}')){
            break;
        }
        switch_token(1);
    }   

    if((current_token->type != BRACKET_CLOSE) || (*(current_token->value) != '}')){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected '}' after function");
        return NULL;
    }

    switch_token(1);
    return parse_start(new_node);
}

ast* parse_include(ast *current_node){
    switch_token(1);

    if(current_token->type != INDICATOR){
        handle_error(current_token, UNEXPECTED_ERROR, "include needs to link to a file");
    }
    ast *new_node = create_node();
    new_node->type = INCLUDE;
    current_node->branch = new_node;
    
    size_t size = strlen(current_token->value)+1;
    new_node->value = calloc(1, size);
    strncpy(new_node->value, current_token->value, size);
    
    switch_token(1);

    if(current_token->type != DELIMITER){
        handle_error(current_token, UNEXPECTED_ERROR, "include needs to link to a file");
        return NULL;
    }
    
    switch_token(1);

    if((current_token->type != INDICATOR) || (strcmp(current_token->value, "ql") != 0)){
        handle_error(current_token, UNEXPECTED_ERROR, "include needs to link to a .ql file");
        return NULL;
    }

    switch_token(1);

    if(current_token->type != END_OF_LINE){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected ';' or linebreak at the end of include");
        return NULL;
    }
    return parse_start(new_node);
}

/*
this function determines if the given indicator token is a type
if it is a type, it checks if it is part of a variable declaration and handles that
*/
ast* parse_type(ast *current_node){
    enum variable_type type;

    if(current_token->type != INDICATOR){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected variable type");
        return NULL;
    }

    if(strcmp(current_token->value, "qubit") == 0){
        type = VAR_QUBIT;
    } else if(strcmp(current_token->value, "bit") == 0){
        type = VAR_BIT;
    } else if(strcmp(current_token->value, "N") == 0){
        type = VAR_NATURAL;
    } else if(strcmp(current_token->value, "Z") == 0){
        type = VAR_INTEGER;
    } else if(strcmp(current_token->value, "int") == 0){
        type = VAR_INTEGER;
    } else if(strcmp(current_token->value, "uint") == 0){
        type = VAR_NATURAL;
    } else if(strcmp(current_token->value, "double") == 0){
        type = VAR_DOUBLE;
    } else if(strcmp(current_token->value, "vector") == 0){
        type = VAR_VECTOR;
    } else if(strcmp(current_token->value, "void") == 0){
        type = VAR_VOID;
    } else {
        return NULL;
    }

    
    ast *new_node = create_node();
    new_node->type = TYPE;
    new_node->var_type = type;
    current_node->branch = new_node;

    switch_token(1);

    // now we check if the next token is an indicator
    // if it is, we assume this is a variable declaration
    if(current_token->type == INDICATOR){
        ast *name_node = create_node();
        name_node->type = NAME;

        size_t size = strlen(current_token->value)+1;
        name_node->name = calloc(1, size);
        strncpy(name_node->name, current_token->value, size);

        new_node->branch = name_node;
        switch_token(1);

        return parse_start(name_node);
    }

    return parse_start(new_node);
}

ast *parse_assign(ast *current_node){
    if(last_eol == NULL){
        handle_error(current_token, MISSING_ERROR, "Assignment unable to be parsed");
        return NULL;
    } else if(in_assign){
        handle_error(current_token, FORBIDDEN_ERROR, "Cannot call assign in an assign");
        return NULL;
    }

    ast *left = last_eol->branch;
    ast *temp_node = create_node();

    ast *new_node = create_node();
    new_node->type = ASSIGN;
    new_node->value = malloc(1);
    *(new_node->value) = *(current_token->value);
    new_node->left = left;

    last_eol->branch = new_node;
    last_eol = NULL;
    
    switch_token(1);
    
    in_assign = 1;
    parse_start(temp_node);
    in_assign = 0;
    
    new_node->right = temp_node->branch;
    free(temp_node);


    // we need to forward to the end of the line
    while(current_token != NULL){
        if(current_token->type == END_OF_LINE){
            break;
        }

        switch_token(1);
    }   

    if(current_token->type != END_OF_LINE){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected ';' after assignment");
        return NULL;
    }  

    return parse_start(new_node);
}

/*
this parses operators these can be operations as well as assigns
*/
ast* parse_operator(ast *current_node){
    if(*(current_token->value) == '='){
        return parse_assign(current_node);
    } 

    return NULL;
}

ast* parse_measure(ast *current_node){
    if(current_token->next_token->type != BRACKET_OPEN) return NULL;
    if(*(current_token->next_token->value) != '(') return NULL;

    ast *new_node = create_node();
    new_node->type = MEASURE;

    current_node->branch = new_node;
    switch_token(2);

    if(current_token->type != INDICATOR){
        handle_error(current_token, 0, "Expected qubit to measure");
        return NULL;
    }

    size_t size = strlen(current_token->value)+1;
    new_node->value = calloc(1, size);
    strncpy(new_node->value, current_token->value, size);

    switch_token(1);

    if(current_token->type != BRACKET_CLOSE) return NULL;
    if(*(current_token->value) != ')') return NULL;

    switch_token(1);

    return parse_start(new_node);
}

ast* parse_call(ast *current_node){
    if(current_token->next_token->type != BRACKET_OPEN) return NULL;
    if(*(current_token->next_token->value) != '(') return NULL;

    ast *new_node = create_node();
    new_node->type = CALL;

    size_t size = strlen(current_token->value)+1;
    new_node->name = calloc(1, size);
    strncpy(new_node->name, current_token->value, size);

    current_node->branch = new_node;
    switch_token(2);

    ast *temp_node = create_node();
    new_node->left = temp_node;
    while(current_token != NULL){
        if(current_token->type == BRACKET_CLOSE){
            if(*(current_token->value) != ')') return NULL;
            switch_token(1);
            break;

        } else if(current_token->type == NUMBER){
            ast *number_node = create_node();
            number_node->type = VALUE;
            temp_node->branch = number_node;

            size_t number_size = strlen(current_token->value)+1;
            number_node->value = calloc(1, number_size);
            strncpy(number_node->value, current_token->value, number_size);

            switch_token(1);

            // seems as though we have encountered a double/float
            if(current_token->type == DELIMITER){
                switch_token(1);
                if(current_token->type == NUMBER){
                    number_node->value = realloc(number_node->value, size+strlen(current_token->value));
                    strcat(number_node->value, current_token->value);
                    switch_token(1);
                }
            }
            temp_node = number_node;
            continue;
            
        } else if(current_token->type == INDICATOR){
            ast *iden_node = create_node();
            iden_node->type = IDENTIFIER;

            size_t name_size = strlen(current_token->value)+1;
            iden_node->name = calloc(1, name_size);
            strncpy(iden_node->name, current_token->value, name_size);

            temp_node->branch = iden_node;
            switch_token(1);
            temp_node = iden_node;
            continue;

        } else if(current_token->type == DELIMITER){
            switch_token(1);
        } else {
            handle_error(current_token, 0, "Expected Number or Identifier in function call");
            return NULL;
        }

    }
    if(new_node->left->branch != NULL){
        temp_node = new_node->left;
        new_node->left = new_node->left->branch;
        free(temp_node);
    }

    return parse_start(new_node);
}

/*
this function is the starting point for all string based commands
it determines what the string means and tries to send it to the apropiate function
*/
ast* parse_indicator(ast *current_node){
    // check if its a type
    ast *res = parse_type(current_node);
    if(res != NULL){
        return res;
    }
    
    // check if its a function definition
    if(strcmp(current_token->value, "def") == 0){
        return parse_function(current_node);
    }
    
    // check if its a measure
    if(strcmp(current_token->value, "measure") == 0){
        return parse_measure(current_node);
    }
    
    // check if its an include
    if(strcmp(current_token->value, "include") == 0){
        return parse_include(current_node);
    }
    
    // check if its a return
    if(strcmp(current_token->value, "return") == 0){
        ast *new_node = create_node();
        new_node->type = RETURN;
        current_node->branch = new_node;
        
        switch_token(1);
        return parse_start(current_node);
    }
    
    // check if its a function call
    res = parse_call(current_node);
    if(res != NULL){
        return res;
    }

    // now we assume we are handling a variable reference aka an identifier
    ast *new_node = create_node();
    new_node->type = IDENTIFIER;

    size_t size = strlen(current_token->value)+1;
    new_node->name = calloc(1, size);
    strncpy(new_node->name, current_token->value, size);

    current_node->branch = new_node;
    switch_token(1);

    return parse_start(current_node);
}

ast* parse_number(ast *current_node){
    ast *new_node = create_node();
    new_node->type = VALUE;
    current_node->branch = new_node;

    size_t size = strlen(current_token->value)+1;
    new_node->value = calloc(1, size);

    strncpy(new_node->value, current_token->value, size);

    switch_token(1);

    // seems as though we have encountered a double/float
    if(current_token->type == DELIMITER){
        switch_token(1);
        if(current_token->type == NUMBER){
            new_node->value = realloc(new_node->value, size+strlen(current_token->value));
            strcat(new_node->value, current_token->value);
            switch_token(1);
        }
    }

    return parse_start(new_node);
}





ast* parse_start(ast *current_node){
    if(!current_token){
        return current_node;
    } else if(!current_node){
        return NULL;
    }

    switch(current_token->type){
        case INDICATOR:
            return parse_indicator(current_node);
        
        case NUMBER:
            return parse_number(current_node);

        case END_OF_LINE:
            last_eol = current_node;
            if(in_assign){
                return current_node;
            }
            switch_token(1);
            return parse_start(current_node);
        
        case OPERATOR:
            return parse_operator(current_node);

        case START:
        case COMMENT:
            switch_token(1);
            return parse_start(current_node);

        case BRACKET_CLOSE:
        case END:
            return current_node;

        default:
            handle_error(current_token, 0, "Not recognised in this context");
        }
    return current_node;
}

ast* generate_ast(token *first_token){
    // initialisations
    current_token = first_token;
    ast *root = create_node();
    in_assign = 0;
    last_eol = root;

    if(parse_start(root) == NULL){
        fprintf(stderr, "handle_input returned NULL\n");
        return NULL;
    }
    return root;
}