#ifndef H_SCANNER_QUALLE
#define H_SCANNER_QUALLE

// enum that defines the token types
enum token_type {
    INDICATOR,
    NUMBER,
    OPERATOR,
    COMMENT,
    BRACKET_OPEN,
    BRACKET_CLOSE,
    DELIMITER,
    END_OF_LINE,
    START,
    END,
    UNKOWN
};

//struct that defines tokens
typedef struct token {
    enum token_type type;
    char* value;
    int line;
    struct token* prev_token;
    struct token* next_token;
} token;

token* get_token(char* buffer);

#endif