#ifndef H_LEXER_QUALLE
#define H_LEXER_QUALLE

// enum that defines the token types
enum token_type {
    T_IDENTIFIER, // any string that starts with a letter
    T_NUMBER, // any number that starts with a digit
    T_OPERATOR, // all kinds of operators and symbols
    T_COMMENT, // start of a comment
    T_BRACKET_OPEN, // any open bracket
    T_BRACKET_CLOSE, // any closed bracket
    T_DELIMITER, // any delimiter
    T_END_OF_LINE, // \n or ;
    T_START,
    T_END,
    T_UNKOWN
};

//struct that defines tokens
typedef struct token {
    enum token_type type;
    char* value;
    int line;
    struct token* next_token;
} token;

// this gets called by main, main also allocates and frees buffer
token* get_token(char* buffer);

#endif