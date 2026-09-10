#ifndef H_ERROR_QUALLE
#define H_ERROR_QUALLE

typedef enum {
    INTERNAL,
    WARNING,
    ERROR,
    FATAL
} severity;

typedef struct {
    severity type;
    int line;
    char *message;
} diagnose;

typedef struct {
    diagnose entries[64];
    int size;
    int warning_count;
    int error_count;
} global_errors;

void init_errors(void);
void print_errors(void); // prints the error list
void check_errors(void);
void add_error_entry(diagnose d);

#endif