#ifndef H_ERROR_QUALLE
#define H_ERROR_QUALLE

// error severity
typedef enum {
    INTERNAL,
    WARNING,
    ERROR,
    FATAL
} severity;

// single error
typedef struct {
    severity type;
    int line;
    char *message;
} diagnose;

// struct to manage all errors
typedef struct {
    diagnose entries[64];
    int size;
    int warning_count;
    int error_count;
} global_errors;

void free_errors(void);
void init_errors(void);
void print_errors(void); // prints the error list
void check_errors(void);
void add_error_entry(severity type, int line, char *message);

#endif