#ifndef H_FLAGS_QUALLE
#define H_FLAGS_QUALLE

#include <stdbool.h>

/*
These are the global flags for QUALLE
They are technically set in the main function.
Due to GCC compilation order shenanigans, we have to define them in the file they are first used.
*/
extern bool ll;
extern bool optimisation;
extern bool print;
extern bool adaptive;
extern bool dynamic;

#endif