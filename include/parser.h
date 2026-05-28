#ifndef H_PARSER_QUALLE
#define H_PARSER_QUALLE

// enum for token types
enum type{
    FUNCTION,
    NAME,
    VALUE,
    IF,
    ELSE,
    WHILE,
    FOR,
    END,
    EMPTY,
    UNKOWN
};
// enum for variable types
enum variable_type{
    QBIT,
    BIT,
    INTEGER,
    NATURAL,
    RATIONAL,
    IRRATIONAL,
    COMPLEX,
    STRING
};

#endif