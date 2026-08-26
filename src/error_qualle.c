#include "error_qualle.h"
#include <stdio.h>
#include <stdlib.h>

static global_errors errors;

void init_errors(){
    errors.size = 0;
    errors.warning_count = 0;
    errors.error_count = 0;
}

void print_errors(){
    if(errors.size <= 0) return;
    if(errors.size > 64) return;

    for(int i = 0; i < errors.size; i++){
        fprintf(stderr, "%i. ", i+1);
        switch (errors.entries[i].type){
        case WARNING:
            if(errors.entries[i].line < 1) fprintf(stderr, "WARNING: %s\n", errors.entries[i].message);
            else fprintf(stderr, "WARNING in line %i: %s\n", errors.entries[i].line, errors.entries[i].message);
            break;

        case ERROR:
            if(errors.entries[i].line < 1) fprintf(stderr, "ERROR: %s\n", errors.entries[i].message);
            else fprintf(stderr, "ERROR in line %i: %s\n", errors.entries[i].line, errors.entries[i].message);
            break;

        case FATAL:
            if(errors.entries[i].line < 1) fprintf(stderr, "FATAL ERROR: %s\n", errors.entries[i].message);
            else fprintf(stderr, "FATAL ERROR in line %i: %s\n", errors.entries[i].line, errors.entries[i].message);
            break;
        
        default:
            fprintf(stderr, "UNKOWN ERROR TYPE\n");
            exit(1);
            break;
        }
    }

    if(errors.size == 64){
        fprintf(stderr, "TOO MANY ERRORS\n");
        exit(1);
    }
}

void check_errors(){
    if(errors.error_count > 0){
        fprintf(stderr, "ABORTING DUE TO %i ERRORS DURING THE LAST PHASE\n", errors.error_count);
        print_errors();
        exit(1);
    }
    if(errors.size >= 64){
        fprintf(stderr, "ABORTING DUE TO TOO MANY ERRRORS\n");
        print_errors();
        exit(1);
    }
    if(errors.size > 0){
        print_errors();
    }
}

void add_error_entry(diagnose d){
    switch(d.type){
        case WARNING:
            errors.warning_count += 1;
            errors.entries[errors.size] = d;
            errors.size += 1;
            break;
        case ERROR:
            errors.error_count += 1;
            errors.entries[errors.size] = d;
            errors.size += 1;
            break;
        case FATAL:
            errors.entries[errors.size] = d;
            errors.size += 1;
            print_errors();
            exit(1);
            break;
        default:
            fprintf(stderr, "UNKOWN ERROR TYPE\n");
            exit(1);
    }
}
