#ifndef H_LEXER_QUALLE
#define H_LEXER_QUALLE

// enum that defines the token types
enum token_type {
    T_IDENTIFIER,
    T_NUMBER,
    T_OPERATOR,
    T_COMMENT,
    T_BRACKET_OPEN,
    T_BRACKET_CLOSE,
    T_DELIMITER,
    T_END_OF_LINE,
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