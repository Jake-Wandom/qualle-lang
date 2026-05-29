#ifndef H_PARSER_QUALLE
#define H_PARSER_QUALLE

// enum for variable types
enum variable_type {
    VAR_QBIT,
    VAR_BIT,
    VAR_BOOL,
    VAR_INTEGER,
    VAR_NATURAL,
    VAR_RATIONAL,
    VAR_IRRATIONAL,
    VAR_COMPLEX,
    VAR_CHAR,
    VAR_STRING
};

// enum for keywords
enum keywords {
    IF,
    ELSE,
    WHILE,
    FOR,
    DEF,
    MAIN
};

#endif