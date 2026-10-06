#include "generator.h"
#include "helper.h"
#include "error_qualle.h"
#include "global_flags.h"

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define QIR_MAJOR_VERSION 2
#define QIR_MINOR_VERSION 0

bool ll = 0;

// to avoid redefinitions we store these globally
static LLVMTypeRef ptr_type;
static LLVMTypeRef void_type;
static LLVMTypeRef i32_type;
static LLVMTypeRef i64_type;
static LLVMTypeRef i1_type;
static LLVMTypeRef double_type;

// the number of qubits that are declared and returned is counted dynamically
static int required_num_qubits = 0;
static int required_num_results = 0;

// dynamic array for measurement and output recording
static LLVMValueRef *result_list = NULL;
static bool measured = 0;

// global variables for function definitions
static LLVMTypeRef measure_type;
static LLVMValueRef measure_function;
static LLVMTypeRef h_type;
static LLVMValueRef h_function;
static LLVMTypeRef x_type;
static LLVMValueRef x_function;
static LLVMTypeRef y_type;
static LLVMValueRef y_function;
static LLVMTypeRef z_type;
static LLVMValueRef z_function;
static LLVMTypeRef rx_type;
static LLVMValueRef rx_function;
static LLVMTypeRef ry_type;
static LLVMValueRef ry_function;
static LLVMTypeRef rz_type;
static LLVMValueRef rz_function;
static LLVMTypeRef s_type;
static LLVMValueRef s_function;
static LLVMTypeRef t_type;
static LLVMValueRef t_function;
static LLVMTypeRef cx_type;
static LLVMValueRef cx_function;
static LLVMTypeRef cy_type;
static LLVMValueRef cy_function;
static LLVMTypeRef cz_type;
static LLVMValueRef cz_function;
static LLVMTypeRef swap_type;
static LLVMValueRef swap_function;
static LLVMTypeRef rxx_type;
static LLVMValueRef rxx_function;
static LLVMTypeRef ryy_type;
static LLVMValueRef ryy_function;
static LLVMTypeRef rzz_type;
static LLVMValueRef rzz_function;

// global variable for the current function
static LLVMValueRef current_function;

int find_variable(variable *var_list, size_t size, LLVMValueRef *llvm){
    for(size_t i = 0; i < size; i++){
        if(var_list[i].llvm == llvm){
            return i;
        }
    }
    return -1;
}

LLVMValueRef make_label(qir_context qir, int number){
    size_t temp_size = 2;
    if(number > 0) temp_size = floor(log10(number))+2;
        
    char *label_name = calloc(1, temp_size);
    if(!label_name){
        diagnose d = {.line = -1, .message = "Failed to allocate memory for label name", .type = FATAL};
        add_error_entry(d);
        return NULL;
    }
    snprintf(label_name, temp_size, "%i", number);

    char *label_identifier = calloc(1, 1+temp_size);
    if(!label_identifier){
        diagnose d = {.line = -1, .message = "Failed to allocate memory for label identifier", .type = FATAL};
        add_error_entry(d);
        return NULL;
    }
    snprintf(label_identifier, 1+temp_size, "r%i", number);

    LLVMTypeRef array_type = LLVMArrayType2(LLVMInt8TypeInContext(qir.context), 1+temp_size);
    LLVMValueRef array = LLVMAddGlobal(qir.module, array_type, label_name);

    LLVMSetLinkage(array, LLVMInternalLinkage);
    LLVMSetGlobalConstant(array, 1);
    LLVMSetInitializer(array, LLVMConstStringInContext2(qir.context, label_identifier, temp_size, 0));

    free(label_name);
    free(label_identifier);
    
    return array;
}

void call_function(qir_context qir, ast *node, char *name){
    LLVMTypeRef one_param[1] = { ptr_type };
    LLVMTypeRef two_param[2] = { ptr_type , ptr_type };

    if(node->llvm == NULL){
        diagnose d = {.line = node->line, .message = "llvm pointer is NULL in function call", .type = ERROR};
        add_error_entry(d);
    }

    if(strcmp(name, "H") == 0){
        LLVMValueRef args[1] = { *(node->llvm) };

        if(h_type == 0){
            h_type = LLVMFunctionType(void_type, one_param, 1, 0);
            h_function = LLVMAddFunction(qir.module, "__quantum__qis__h__body", h_type);
        }
        LLVMBuildCall2(qir.builder, h_type, h_function, args, 1, "");
        if(print) printf("%s CALL with %s:%p\n",name, node->name ,(void*)node->llvm);
        
    } else if(strcmp(name, "X") == 0){
        LLVMValueRef args[1] = { *(node->llvm) };

        if(x_type == 0){
            x_type = LLVMFunctionType(void_type, one_param, 1, 0);
            x_function = LLVMAddFunction(qir.module, "__quantum__qis__x__body", x_type);
        }
        LLVMBuildCall2(qir.builder, x_type, x_function, args, 1, "");
        if(print) printf("%s CALL with %s:%p\n",name, node->name ,(void*)node->llvm);

    } else if(strcmp(name, "Y") == 0){
        LLVMValueRef args[1] = { *(node->llvm) };

        if(y_type == 0){
            y_type = LLVMFunctionType(void_type, one_param, 1, 0);
            y_function = LLVMAddFunction(qir.module, "__quantum__qis__y__body", y_type);
        }
        LLVMBuildCall2(qir.builder, y_type, y_function, args, 1, "");
        if(print) printf("%s CALL with %s:%p\n",name, node->name ,(void*)node->llvm);

    } else if(strcmp(name, "Z") == 0){
        LLVMValueRef args[1] = { *(node->llvm) };

        if(z_type == 0){
            z_type = LLVMFunctionType(void_type, one_param, 1, 0);
            z_function = LLVMAddFunction(qir.module, "__quantum__qis__z__body", z_type);
        }
        LLVMBuildCall2(qir.builder, z_type, z_function, args, 1, "");
        if(print) printf("%s CALL with %s:%p\n",name, node->name ,(void*)node->llvm);

    } else if(strcmp(name, "RX") == 0){
        LLVMValueRef args[1] = { *(node->llvm) };

        if(rx_type == 0){
            rx_type = LLVMFunctionType(void_type, one_param, 1, 0);
            rx_function = LLVMAddFunction(qir.module, "__quantum__qis__rx__body", rx_type);
        }
        LLVMBuildCall2(qir.builder, rx_type, rx_function, args, 1, "");
        if(print) printf("%s CALL with %s:%p\n",name, node->name ,(void*)node->llvm);

    } else if(strcmp(name, "RY") == 0){
        LLVMValueRef args[1] = { *(node->llvm) };

        if(ry_type == 0){
            ry_type = LLVMFunctionType(void_type, one_param, 1, 0);
            ry_function = LLVMAddFunction(qir.module, "__quantum__qis__ry__body", ry_type);
        }
        LLVMBuildCall2(qir.builder, ry_type, ry_function, args, 1, "");
        if(print) printf("%s CALL with %s:%p\n",name, node->name ,(void*)node->llvm);

    } else if(strcmp(name, "RZ") == 0){
        LLVMValueRef args[1] = { *(node->llvm) };

        if(rz_type == 0){
            rz_type = LLVMFunctionType(void_type, one_param, 1, 0);
            rz_function = LLVMAddFunction(qir.module, "__quantum__qis__rz__body", rz_type);
        }
        LLVMBuildCall2(qir.builder, rz_type, rz_function, args, 1, "");
        if(print) printf("%s CALL with %s:%p\n",name, node->name ,(void*)node->llvm);

    } else if(strcmp(name, "S") == 0){
        LLVMValueRef args[1] = { *(node->llvm) };

        if(s_type == 0){
            s_type = LLVMFunctionType(void_type, one_param, 1, 0);
            s_function = LLVMAddFunction(qir.module, "__quantum__qis__s__body", s_type);
        }
        LLVMBuildCall2(qir.builder, s_type, s_function, args, 1, "");
        if(print) printf("%s CALL with %s:%p\n",name, node->name ,(void*)node->llvm);

    } else if(strcmp(name, "T") == 0){
        LLVMValueRef args[1] = { *(node->llvm) };

        if(t_type == 0){
            t_type = LLVMFunctionType(void_type, one_param, 1, 0);
            t_function = LLVMAddFunction(qir.module, "__quantum__qis__t__body", t_type);
        }
        LLVMBuildCall2(qir.builder, t_type, t_function, args, 1, "");
        if(print) printf("%s CALL with %s:%p\n",name, node->name ,(void*)node->llvm);

    } else if((strcmp(name, "CNOT") == 0) || (strcmp(name, "CX") == 0)){
        LLVMValueRef args[2] = { *(node->llvm), *(node->branch->llvm) };

        if(cx_type == 0){
            cx_type = LLVMFunctionType(void_type, two_param, 2, 0);
            cx_function = LLVMAddFunction(qir.module, "__quantum__qis__cx__body", cx_type);
        }
        LLVMBuildCall2(qir.builder, cx_type, cx_function, args, 2, "");
        if(print) printf("%s CALL with %s:%p\n",name, node->name ,(void*)node->llvm);
    } else if(strcmp(name, "CY") == 0){
        LLVMValueRef args[2] = { *(node->llvm), *(node->branch->llvm) };

        if(cy_type == 0){
            cy_type = LLVMFunctionType(void_type, two_param, 2, 0);
            cy_function = LLVMAddFunction(qir.module, "__quantum__qis__cy__body", cy_type);
        }
        LLVMBuildCall2(qir.builder, cy_type, cy_function, args, 2, "");
        if(print) printf("%s CALL with %s:%p\n",name, node->name ,(void*)node->llvm);
    } else if(strcmp(name, "CZ") == 0){
        LLVMValueRef args[2] = { *(node->llvm), *(node->branch->llvm) };

        if(cz_type == 0){
            cz_type = LLVMFunctionType(void_type, two_param, 2, 0);
            cz_function = LLVMAddFunction(qir.module, "__quantum__qis__cz__body", cz_type);
        }
        LLVMBuildCall2(qir.builder, cz_type, cz_function, args, 2, "");
        if(print) printf("%s CALL with %s:%p\n",name, node->name ,(void*)node->llvm);
    } else if(strcmp(name, "SWAP") == 0){
        LLVMValueRef args[2] = { *(node->llvm), *(node->branch->llvm) };

        if(swap_type == 0){
            swap_type = LLVMFunctionType(void_type, two_param, 2, 0);
            swap_function = LLVMAddFunction(qir.module, "__quantum__qis__swap__body", swap_type);
        }
        LLVMBuildCall2(qir.builder, swap_type, swap_function, args, 2, "");
        if(print) printf("%s CALL with %s:%p\n",name, node->name ,(void*)node->llvm);
    } else if(strcmp(name, "RXX") == 0){
        LLVMValueRef args[2] = { *(node->llvm), *(node->branch->llvm) };

        if(rxx_type == 0){
            rxx_type = LLVMFunctionType(void_type, two_param, 2, 0);
            rxx_function = LLVMAddFunction(qir.module, "__quantum__qis__rxx__body", rxx_type);
        }
        LLVMBuildCall2(qir.builder, rxx_type, rxx_function, args, 2, "");
        if(print) printf("%s CALL with %s:%p\n",name, node->name ,(void*)node->llvm);
    } else if(strcmp(name, "RYY") == 0){
        LLVMValueRef args[2] = { *(node->llvm), *(node->branch->llvm) };

        if(ryy_type == 0){
            ryy_type = LLVMFunctionType(void_type, two_param, 2, 0);
            ryy_function = LLVMAddFunction(qir.module, "__quantum__qis__ryy__body", ryy_type);
        }
        LLVMBuildCall2(qir.builder, ryy_type, ryy_function, args, 2, "");
        if(print) printf("%s CALL with %s:%p\n",name, node->name ,(void*)node->llvm);
    } else if(strcmp(name, "RZZ") == 0){
        LLVMValueRef args[2] = { *(node->llvm), *(node->branch->llvm) };

        if(rzz_type == 0){
            rzz_type = LLVMFunctionType(void_type, two_param, 2, 0);
            rzz_function = LLVMAddFunction(qir.module, "__quantum__qis__rzz__body", rzz_type);
        }
        LLVMBuildCall2(qir.builder, rzz_type, rzz_function, args, 2, "");
        if(print) printf("%s CALL with %s:%p\n",name, node->name ,(void*)node->llvm);
    } 

}

LLVMTypeRef get_type(enum variable_type type){
    switch(type){
        case VAR_QUBIT:
            return ptr_type;
        case VAR_BIT:
            return i1_type;
        case VAR_NATURAL:
        case VAR_INTEGER:
            return i64_type;
        case VAR_VOID:
            return void_type;
        case VAR_DOUBLE:
            return double_type;
    }
}

int add_value(qir_context qir, enum variable_type type, LLVMValueRef value, LLVMValueRef *llvm){
    diagnose d;
    switch(type){
        case VAR_QUBIT:
            *llvm = LLVMConstIntToPtr(LLVMConstInt(i64_type, required_num_qubits, 0), ptr_type);
            required_num_qubits++;
            break;

        case VAR_BIT:
            value = LLVMBuildIntCast2(qir.builder, value, i1_type, 1, "");
            *llvm = LLVMBuildAlloca(qir.builder, i1_type, "");
            LLVMBuildStore(qir.builder, value, *llvm);
            break;

        case VAR_INTEGER:
            value = LLVMBuildIntCast2(qir.builder, value, i64_type, 1, "");
            *llvm = LLVMBuildAlloca(qir.builder, i64_type, "");
            LLVMBuildStore(qir.builder, value, *llvm);
            break;

        case VAR_DOUBLE:
            *llvm = LLVMBuildAlloca(qir.builder, double_type, "");
            LLVMBuildStore(qir.builder, value, *llvm);
            break;

        default:
            d = (diagnose){.line = -1, .message = "This type is not supported", .type = ERROR};
            add_error_entry(d);
            return -1;
    }

    return 0;
}

LLVMValueRef determine_bool(qir_context qir, ast *node){
    if(node->type != BOOLOP){
        diagnose d = {.line = node->line, .message = "Expected boolean operation", .type = INTERNAL};
        add_error_entry(d);
        return NULL;
    }

    LLVMValueRef condition;
    switch(*(node->value)){
        case '|':
        case '&':
        case '<':
        case '>':
        case '=':
            break;

        case '+':
        case '-':
        case '*':
        case '/':
        case '^':
        case '%':
            break;
    }
}

LLVMValueRef determine_value(qir_context qir, ast *node){
    switch(node->type){
        case BINOP:
            LLVMValueRef left = determine_value(qir, node->left);
            LLVMValueRef right = determine_value(qir, node->right);
            switch(*(node->value)){
                case '+':
                    return LLVMBuildAdd(qir.builder, left, right, "");
                case '-':
                    return LLVMBuildSub(qir.builder, left, right, "");
                case '*':
                    return LLVMBuildMul(qir.builder, left, right, "");
                case '/':
                    return LLVMBuildSDiv(qir.builder, left, right, "");
                case '%':
                    return LLVMBuildSRem(qir.builder, left, right, "");
            }
            break;
        case IDENTIFIER:
            if(*(node->llvm) == NULL){
                diagnose d = {.line = node->line, .message = "LLVM Null pointer", .type = INTERNAL};
                add_error_entry(d);
            }
            return LLVMBuildLoad2(qir.builder, get_type(node->resolved_type), *(node->llvm), "");
        case VALUE:
            if(node->resolved_type == VAR_QUBIT){
                return (LLVMValueRef)0;
            } else if(node->resolved_type == VAR_DOUBLE){
                fprintf(stderr, "DOUBLE HERE: %d\n");
                return LLVMConstInt(double_type, strtod(node->value, NULL), 0);
            } else {
                return LLVMConstInt(get_type(node->resolved_type), strtol(node->value, NULL, 10), 0);
            }
    }
}

void generate_instructions(qir_context qir, ast *node){
    if(!node) return;
    if(!adaptive && measured && (node->type != MEASURE)){
        diagnose d = {.line = node->line, .message = "A BASE profile compliant program does not allow any operations after a measurement", .type = ERROR};
        add_error_entry(d);
        return;
    }

    LLVMValueRef value;
    switch(node->type){
        case CALL:
            call_function(qir, node->left, node->name);
            break;

        case MEASURE:
            measured = 1;
            required_num_results++;
            result_list = realloc(result_list, required_num_results*sizeof(LLVMValueRef));

            result_list[required_num_results-1] = *(node->llvm);
            if(adaptive){
                LLVMValueRef mz_args[2] = { *(node->llvm), *(node->llvm)};
                LLVMBuildCall2(qir.builder, measure_type, measure_function, mz_args, 2, "");
            }

            if(print) printf("MEASUREMENT with %s:%p\n",node->name ,(void*)node->llvm);
            break;

        case TYPE:
            diagnose d = {.line = node->line, .message = "The TYPE node type is currently not supported", .type = FATAL};
            add_error_entry(d);
            return;
            break;

        case NAME:
            if(node->llvm == NULL) {
                return;
            }
            if(add_value(qir, node->resolved_type, 0, node->llvm) == -1) return;

            if(print) printf("NEW VAR %s:%p\n",node->name ,(void*)node->llvm);
            break;

        case ASSIGN:
            value = determine_value(qir, node->right);
            if(add_value(qir, node->left->resolved_type, value, node->left->llvm) == -1) return;
            if(node->left->resolved_type == VAR_QUBIT){
                if(value == 1){
                    call_function(qir, node->left->branch, "X");
                }
            }

            if(print) printf("NEW VAR %s:%p\n",node->left->name ,(void*)node->left->llvm);
            break;
        case FUNCTION:
            break;
        case LOOP:
            if(*(node->value) == 'f'){
                // for loop building blocks
                LLVMBasicBlockRef for_condition_block = LLVMAppendBasicBlockInContext(qir.context, current_function, "for_condition");
                LLVMBasicBlockRef for_body_block = LLVMAppendBasicBlockInContext(qir.context, current_function, "for_body");
                LLVMBasicBlockRef for_latch_block = LLVMAppendBasicBlockInContext(qir.context, current_function, "for_latch");
                LLVMBasicBlockRef for_exit_block = LLVMAppendBasicBlockInContext(qir.context, current_function, "for_exit");

                // TODO determine value
                LLVMValueRef init = LLVMConstInt(i64_type, 0, 0);
                LLVMBuildBr(qir.builder, for_condition_block);
                LLVMPositionBuilderAtEnd(qir.builder, for_condition_block);

                LLVMValueRef phi = LLVMBuildPhi(qir.builder, i64_type, "i");
                LLVMAddIncoming(phi, &init, &for_condition_block, 1); 

                LLVMValueRef limit = LLVMConstInt(i64_type, 1, 0); // TODO
                LLVMValueRef condition = LLVMBuildICmp(qir.builder, LLVMIntSLT, phi, limit, "i_lt_n");
                LLVMBuildCondBr(qir.builder, condition, for_body_block, for_exit_block);

                LLVMPositionBuilderAtEnd(qir.builder, for_body_block);
                generate_instructions(qir, node->right);
                LLVMBuildBr(qir.builder, for_latch_block);

                LLVMPositionBuilderAtEnd(qir.builder, for_latch_block);
                LLVMValueRef next = LLVMBuildAdd(qir.builder, LLVMConstInt(i64_type, 1, 0), phi, "next");
                LLVMBuildBr(qir.builder, for_condition_block);

                LLVMAddIncoming(phi, &next, &for_latch_block, 1);

                LLVMPositionBuilderAtEnd(qir.builder, for_exit_block);

            } else if(*(node->value) == 'w'){
                // while loop building blocks
                LLVMBasicBlockRef while_condition_block = LLVMAppendBasicBlockInContext(qir.context, current_function, "while_condition");
                LLVMBasicBlockRef while_body_block = LLVMAppendBasicBlockInContext(qir.context, current_function, "while_body");
                LLVMBasicBlockRef while_exit_block = LLVMAppendBasicBlockInContext(qir.context, current_function, "while_exit");

                LLVMPositionBuilderAtEnd(qir.builder, while_condition_block);
                LLVMValueRef condition = determine_bool(qir, node->left);
                LLVMBuildCondBr(qir.builder, condition, while_body_block, while_exit_block);

                // body
                LLVMPositionBuilderAtEnd(qir.builder, while_body_block);
                generate_instructions(qir, node->right);
                LLVMBuildBr(qir.builder, while_condition_block);

                // exit while
                LLVMPositionBuilderAtEnd(qir.builder, while_exit_block);
                
            } else {
                diagnose d = {.line = node->line, .message = "Unable to determine loop type", .type = INTERNAL};
                add_error_entry(d);
                return;
            }
            break;
        case CONDITIONAL:
            LLVMBasicBlockRef if_condition_block = LLVMAppendBasicBlockInContext(qir.context, current_function, "if_condition");
            LLVMBasicBlockRef if_body_block = LLVMAppendBasicBlockInContext(qir.context, current_function, "if_body");
            LLVMBasicBlockRef else_body_block = LLVMAppendBasicBlockInContext(qir.context, current_function, "else_body");
            LLVMBasicBlockRef continue_block = LLVMAppendBasicBlockInContext(qir.context, current_function, "continue");

            LLVMPositionBuilderAtEnd(qir.builder, if_condition_block);
            LLVMValueRef condition = determine_bool(qir, node->left);
            if(*(node->value) == 'i'){
                LLVMBuildCondBr(qir.builder, condition, if_body_block, continue_block);
                LLVMPositionBuilderAtEnd(qir.builder, if_body_block);
                generate_instructions(qir, node->right);
                LLVMBuildBr(qir.builder, continue_block);

            } else {
                LLVMBuildCondBr(qir.builder, condition, if_body_block, else_body_block);
                LLVMPositionBuilderAtEnd(qir.builder, if_body_block);
                generate_instructions(qir, node->right->left);
                LLVMPositionBuilderAtEnd(qir.builder, else_body_block);
                generate_instructions(qir, node->right->right);
                LLVMBuildBr(qir.builder, continue_block);
            }
            LLVMPositionBuilderAtEnd(qir.builder, continue_block);
            break;
        default:
            break;
    }
    generate_instructions(qir, node->branch);
}


FILE *generate_QIR(ast *root){
    // setup for qir_context
    qir_context qir;
    measured = 0;

    // general LLVM setup
    qir.context = LLVMContextCreate();
    qir.module = LLVMModuleCreateWithNameInContext("QUALLE_module", qir.context);
    qir.builder = LLVMCreateBuilderInContext(qir.context);

    i32_type = LLVMInt32TypeInContext(qir.context);
    i64_type = LLVMInt64TypeInContext(qir.context);
    i1_type = LLVMInt1TypeInContext(qir.context);
    void_type = LLVMVoidTypeInContext(qir.context);
    ptr_type = LLVMPointerTypeInContext(qir.context, 0);
    double_type = LLVMDoubleTypeInContext(qir.context);
    LLVMTypeRef one_param[1] = { ptr_type };
    LLVMTypeRef two_param[2] = { ptr_type , ptr_type };


    // analyse the ast
    if(print) printf("ANALYSER PHASE:\n");
    size_t size = count_nodes(root);
    variable *variable_list = analyse_ast(root);
    if(variable_list == NULL) goto dispose;
    if(print) print_ast(root, 1);

    if(print) printf("\n");

    if(print) printf("GENERATOR PHASE:\n");

    
    // define basic functions
    // main function
    LLVMTypeRef main_type = LLVMFunctionType(i64_type, NULL, 0, 0);
    LLVMValueRef main_function = LLVMAddFunction(qir.module, "main", main_type);
    current_function = main_function;
    
    // measure function
    measure_type = LLVMFunctionType(void_type, two_param, 2, 0);
    measure_function = LLVMAddFunction(qir.module, "__quantum__qis__mz__body", measure_type);

    // record output
    LLVMTypeRef result_type = LLVMFunctionType(void_type, two_param, 2, 0);
    LLVMValueRef result_function = LLVMAddFunction(qir.module, "__quantum__rt__result_record_output", result_type);

    // init function
    LLVMTypeRef init_type = LLVMFunctionType(void_type, one_param, 1, 0);
    LLVMValueRef init_function = LLVMAddFunction(qir.module, "__quantum__rt__initialize", init_type);

    
    // build block structure
    LLVMBasicBlockRef entry_block = LLVMAppendBasicBlockInContext(qir.context, main_function, "entry");
    LLVMBasicBlockRef body_block = LLVMAppendBasicBlockInContext(qir.context, main_function, "body");
    LLVMBasicBlockRef measure_block = LLVMAppendBasicBlockInContext(qir.context, main_function, "measure");
    LLVMBasicBlockRef output_block = LLVMAppendBasicBlockInContext(qir.context, main_function, "output");


    // entry block + init function
    LLVMPositionBuilderAtEnd(qir.builder, entry_block);
    LLVMValueRef init_args[1] = { LLVMConstNull(ptr_type) };
    LLVMBuildCall2(qir.builder, init_type, init_function, init_args, 1, "");
    LLVMBuildBr(qir.builder, body_block);

    // body block + generate instructions
    LLVMPositionBuilderAtEnd(qir.builder, body_block);

    if(print) printf("\nGENERATOR:\n");
    generate_instructions(qir, root);

    if(adaptive == 0) LLVMBuildBr(qir.builder, measure_block);

    // measure block
    // this block is only used in a base profile program
    if(adaptive == 0){
        LLVMPositionBuilderAtEnd(qir.builder, measure_block);
    
        for(int i = 0; i < required_num_results; i++){
            LLVMValueRef mz_args[2] = { result_list[i], result_list[i]};
            LLVMBuildCall2(qir.builder, measure_type, measure_function, mz_args, 2, "");
        }
    
    }
    LLVMBuildBr(qir.builder, output_block);

    // output block
    LLVMPositionBuilderAtEnd(qir.builder, output_block);
    
    for(int i = 0; i < required_num_results; i++){
        LLVMValueRef label = make_label(qir, i);
        LLVMValueRef rs_args[2] = { result_list[i], label};

        LLVMBuildCall2(qir.builder, result_type, result_function, rs_args, 2, "");
    }
    
    // return 0
    LLVMBuildRet(qir.builder, LLVMConstInt(i64_type, 0, 0));

    // convert the number of qubits and results to strings
    int len_qubits = floor(log10(required_num_qubits)) + 1; // this calculates the number of chars
    if(required_num_qubits == 0) len_qubits = 1;
    char *str_qubits = malloc(len_qubits+1);
    snprintf(str_qubits, len_qubits+1, "%d", required_num_qubits);

    int len_results = floor(log10(required_num_results)) + 1;
    if(required_num_results == 0) len_results = 1;
    char *str_results = malloc(len_results+1);
    snprintf(str_results, len_results+1, "%d", required_num_results);

    // define all attributes
    LLVMAttributeRef entryAttribute = LLVMCreateStringAttribute(qir.context, "entry_point", 11, "", 0);
    LLVMAttributeRef labelingAttribute = LLVMCreateStringAttribute(qir.context, "output_labeling_schema", 22, "", 0);
    LLVMAttributeRef profileAttribute = LLVMCreateStringAttribute(qir.context, "qir_profiles", 12, "base_profile", 12);
    LLVMAttributeRef qubitsAttribute = LLVMCreateStringAttribute(qir.context, "required_num_qubits", 19, str_qubits, len_qubits);
    LLVMAttributeRef resultsAttribute = LLVMCreateStringAttribute(qir.context, "required_num_results", 20, str_results, len_results);

    LLVMAddAttributeAtIndex(main_function, LLVMAttributeFunctionIndex, entryAttribute);
    LLVMAddAttributeAtIndex(main_function, LLVMAttributeFunctionIndex, labelingAttribute);
    LLVMAddAttributeAtIndex(main_function, LLVMAttributeFunctionIndex, profileAttribute);
    LLVMAddAttributeAtIndex(main_function, LLVMAttributeFunctionIndex, qubitsAttribute);
    LLVMAddAttributeAtIndex(main_function, LLVMAttributeFunctionIndex, resultsAttribute);

    // measurement attribute
    unsigned kind = LLVMGetEnumAttributeKindForName("writeonly", 9);
    LLVMAttributeRef irrevAttribute = LLVMCreateStringAttribute(qir.context, "irreversible", 12, "", 0);
    LLVMAttributeRef writeonlyAttribute = LLVMCreateEnumAttribute(qir.context, kind, 0);
    
    LLVMAddAttributeAtIndex(measure_function, 2, writeonlyAttribute);
    LLVMAddAttributeAtIndex(measure_function, LLVMAttributeFunctionIndex, irrevAttribute);


    // add module flags these are QIR specific
    LLVMMetadataRef meta_major = LLVMValueAsMetadata(LLVMConstInt(i32_type, QIR_MAJOR_VERSION, 0));
    LLVMMetadataRef meta_minor = LLVMValueAsMetadata(LLVMConstInt(i32_type, QIR_MINOR_VERSION, 0));
    LLVMMetadataRef meta_dynamic_qu = LLVMValueAsMetadata(LLVMConstInt(i1_type, adaptive, 0));
    LLVMMetadataRef meta_dynamic_res = LLVMValueAsMetadata(LLVMConstInt(i1_type, adaptive, 0));

    LLVMAddModuleFlag(qir.module, LLVMModuleFlagBehaviorError, "qir_major_version", 17, meta_major);
    LLVMAddModuleFlag(qir.module, 6, "qir_minor_version", 17, meta_minor);
    LLVMAddModuleFlag(qir.module, LLVMModuleFlagBehaviorError, "dynamic_qubit_management", 24, meta_dynamic_qu);
    LLVMAddModuleFlag(qir.module, LLVMModuleFlagBehaviorError, "dynamic_result_management", 25, meta_dynamic_res);

    free(str_qubits);
    free(str_results);
    // release the list
    free(result_list);
    free_var_list(variable_list, size);

    // string for error handling
    char *error = NULL;
    FILE *output = NULL;
    // if bitcode is set, we generate a .bc file
    if(!ll){
        if(LLVMWriteBitcodeToFile(qir.module, "output.bc") != 0){
            diagnose d = {.line = -1, .message = "Error while writing .bc file", .type = ERROR};
            add_error_entry(d);
            return NULL;
        }
        output = fopen("output.bc", "r+");
    } else { // if not, we generate a human readable .ll file
        if(LLVMPrintModuleToFile(qir.module, "output.ll", &error) != 0){
            diagnose d = {.line = -1, .message = "Error while writing .ll file", .type = ERROR};
            add_error_entry(d);
            LLVMDisposeMessage(error);
            return NULL;
        }
        output = fopen("output.ll", "r+");
    }
    
    if(!output){
        diagnose d = {.line = -1, .message = "Unable to open output file", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }

    dispose:
    LLVMDisposeBuilder(qir.builder);
    LLVMDisposeModule(qir.module);
    LLVMContextDispose(qir.context);
    check_errors();
    return output;
}   