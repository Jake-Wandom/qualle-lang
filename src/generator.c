#include "generator.h"
#include "helper.h"
#include "error_qualle.h"
#include "global_flags.h"
#include "quantum_gates.h"

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Transforms/PassBuilder.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define QIR_MAJOR_VERSION 2
#define QIR_MINOR_VERSION 0

bool ll = 0;
bool optimisation = 0;

// global context
static context *ctx;
// to avoid redefinitions we store these globally
static LLVMTypeRef ptr_type;
static LLVMTypeRef void_type;
static LLVMTypeRef i32_type;
static LLVMTypeRef i64_type;
static LLVMTypeRef i1_type;
static LLVMTypeRef double_type;

// global variables for function definitions
static LLVMTypeRef measure_type;
static LLVMValueRef measure_function;
static LLVMTypeRef read_res_type;
static LLVMValueRef read_res_function;
static LLVMTypeRef main_type;
static LLVMValueRef main_function;

// global variable for the current function
static LLVMValueRef current_function;

LLVMValueRef make_label(qir_context *qir, int number){
    size_t temp_size = 2;
    if(number > 0) temp_size = floor(log10(number))+2;
        
    char *label_name = calloc(1, temp_size);
    if(!label_name){
        add_error_entry(FATAL, -1 , "Failed to allocate memory for label name");
        return NULL;
    }
    snprintf(label_name, temp_size, "%i", number);

    char *label_identifier = calloc(1, 1+temp_size);
    if(!label_identifier){
        add_error_entry(FATAL, -1, "Failed to allocate memory for label identifier");
        return NULL;
    }
    snprintf(label_identifier, 1+temp_size, "r%i", number);

    LLVMTypeRef array_type = LLVMArrayType2(LLVMInt8TypeInContext(qir->context), 1+temp_size);
    LLVMValueRef array = LLVMAddGlobal(qir->module, array_type, label_name);

    LLVMSetLinkage(array, LLVMInternalLinkage);
    LLVMSetGlobalConstant(array, 1);
    LLVMSetInitializer(array, LLVMConstStringInContext2(qir->context, label_identifier, temp_size, 0));

    free(label_name);
    free(label_identifier);
    
    return array;
}

void emit_gate(qir_context *qir, int gate_index, LLVMValueRef *args, LLVMTypeRef *param){
    if(qir->gate_type[gate_index] == 0){
        qir->gate_type[gate_index] = LLVMFunctionType(void_type, param, q_gates[gate_index].args, 0);
        qir->gate_fn[gate_index] = LLVMAddFunction(qir->module, q_gates[gate_index].qir_name, qir->gate_type[gate_index]);
    }

    LLVMBuildCall2(qir->builder, qir->gate_type[gate_index], qir->gate_fn[gate_index], args, q_gates[gate_index].args, "");
}

LLVMTypeRef get_type(enum variable_type type){
    switch(type){
        case VAR_QUBIT:
            return ptr_type;
        case VAR_BIT:
            return i1_type;
        case VAR_UINTEGER:
        case VAR_INTEGER:
            return i64_type;
        case VAR_VOID:
            return void_type;
        case VAR_DOUBLE:
            return double_type;
        default:
            add_error_entry(FATAL, -1, "Unknown type during generator phase");
            return 0;
    }
}

LLVMValueRef coerce(qir_context *qir, LLVMValueRef val, enum variable_type old_type, enum variable_type new_type){
    if(old_type == new_type) return val;
    if(new_type == VAR_DOUBLE){
        if(old_type == VAR_UINTEGER){
            return LLVMBuildUIToFP(qir->builder, val, get_type(new_type), "");
        }
        return LLVMBuildSIToFP(qir->builder, val, get_type(new_type), "");
    }
    if(new_type == VAR_UINTEGER && old_type == VAR_INTEGER){
        return LLVMBuildSelect(qir->builder, LLVMBuildICmp(qir->builder, LLVMIntSLE, val, LLVMConstInt(i64_type, 0, 0), ""), LLVMBuildNeg(qir->builder, val, ""), val, "");
    }
    if(new_type == VAR_BIT){
        LLVMBool is_signed = 0;
        if(old_type == VAR_INTEGER) is_signed = 1;
        return LLVMBuildIntCast2(qir->builder, val, get_type(new_type), is_signed, "");
    }
    return val;
}

LLVMValueRef generate_binary(qir_context *qir, char *op, enum variable_type common_type, LLVMValueRef left, LLVMValueRef right){
    if(strcmp("+", op) == 0){
        if(common_type == VAR_DOUBLE){
            return LLVMBuildFAdd(qir->builder, left, right, "");
        }
        return LLVMBuildAdd(qir->builder, left, right, "");

    } else if (strcmp("-", op) == 0) {
        if(common_type == VAR_DOUBLE){
            return LLVMBuildFSub(qir->builder, left, right, "");
        }
        return LLVMBuildSub(qir->builder, left, right, "");

    } else if (strcmp("*", op) == 0) {
        if(common_type == VAR_DOUBLE){
            return LLVMBuildFMul(qir->builder, left, right, "");
        }
        return LLVMBuildMul(qir->builder, left, right, "");

    } else if (strcmp("/", op) == 0) {
        if(common_type == VAR_DOUBLE){
            return LLVMBuildFDiv(qir->builder, left, right, "");
        }
        if(common_type == VAR_UINTEGER){
            return LLVMBuildUDiv(qir->builder, left, right, "");
        }
        return LLVMBuildSDiv(qir->builder, left, right, "");

    } else if(strcmp("%", op) == 0){
        if(common_type == VAR_UINTEGER){
            return LLVMBuildURem(qir->builder, left, right, "");
        }
        return LLVMBuildSRem(qir->builder, left, right, "");

    } else if(strcmp("^", op) == 0){
       
    } else if(strcmp("<", op) == 0){
        if(common_type == VAR_DOUBLE){
            return LLVMBuildFCmp(qir->builder, LLVMRealOLT, left, right, "");
        }
        if(common_type == VAR_UINTEGER){
            return LLVMBuildICmp(qir->builder, LLVMIntULT, left, right, "");
        }
        return LLVMBuildICmp(qir->builder, LLVMIntSLT, left, right, "");

    } else if (strcmp("<=", op) == 0) {
        if(common_type == VAR_DOUBLE){
            return LLVMBuildFCmp(qir->builder, LLVMRealOLE, left, right, "");
        }
        if(common_type == VAR_UINTEGER){
            return LLVMBuildICmp(qir->builder, LLVMIntULE, left, right, "");
        }
        return LLVMBuildICmp(qir->builder, LLVMIntSLE, left, right, "");

    } else if (strcmp(">", op) == 0) {
        if(common_type == VAR_DOUBLE){
            return LLVMBuildFCmp(qir->builder, LLVMRealOGT, left, right, "");
        }
        if(common_type == VAR_UINTEGER){
            return LLVMBuildICmp(qir->builder, LLVMIntUGT, left, right, "");
        }
        return LLVMBuildICmp(qir->builder, LLVMIntSGT, left, right, "");

    } else if (strcmp(">=", op) == 0) {
        if(common_type == VAR_DOUBLE){
            return LLVMBuildFCmp(qir->builder, LLVMRealOGE, left, right, "");
        }
        if(common_type == VAR_UINTEGER){
            return LLVMBuildICmp(qir->builder, LLVMIntUGE, left, right, "");
        }
        return LLVMBuildICmp(qir->builder, LLVMIntSGE, left, right, "");

    } else if(strcmp("==", op) == 0){
        if(common_type == VAR_DOUBLE){
            return LLVMBuildFCmp(qir->builder, LLVMRealOEQ, left, right, "");
        }
       return LLVMBuildICmp(qir->builder, LLVMIntEQ, left, right, "");

    } else if (strcmp("!=", op) == 0) {
        if(common_type == VAR_DOUBLE){
            return LLVMBuildFCmp(qir->builder, LLVMRealUNE, left, right, "");
        }
        return LLVMBuildICmp(qir->builder, LLVMIntNE, left, right, "");

    } else if(strcmp("&&", op) == 0){
        return LLVMBuildAnd(qir->builder, left, right, "");

    } else if (strcmp("||", op) == 0) {
        return LLVMBuildOr(qir->builder, left, right, "");
    }
    return 0;
}


LLVMValueRef generate_expression(qir_context *qir, ast *node){
    LLVMValueRef ret = 0;
    switch(node->type){
        case UNOP:
            if(node->left->resolved_type == VAR_BIT){
                LLVMValueRef res = generate_expression(qir, node->left);
                if(res == 0) break;
                ret = LLVMBuildNot(qir->builder, res, "");
            } else {
                LLVMValueRef res = generate_expression(qir, node->left);
                if(res == 0) break;
                if(node->left->resolved_type == VAR_DOUBLE){
                    ret = LLVMBuildFNeg(qir->builder, res, "");
                } else {
                    ret = LLVMBuildNeg(qir->builder, res, "");
                }
            }
            break;
        case BOOLOP: {
            enum variable_type common_type = binop_type("+", node->left->resolved_type, node->right->resolved_type);

            LLVMValueRef left = coerce(qir, generate_expression(qir, node->left), node->left->resolved_type, common_type);
            LLVMValueRef right = coerce(qir, generate_expression(qir, node->right), node->right->resolved_type, common_type);

            ret = generate_binary(qir, node->value, common_type, left, right);
            break;
            }
        case BINOP: {
            enum variable_type common_type = binop_type(node->value, node->left->resolved_type, node->right->resolved_type);

            LLVMValueRef left = coerce(qir, generate_expression(qir, node->left), node->left->resolved_type, common_type);
            LLVMValueRef right = coerce(qir, generate_expression(qir, node->right), node->right->resolved_type, common_type);

            ret = generate_binary(qir, node->value, common_type, left, right);
            break;
            }
        case IDENTIFIER:
            if(node->resolved_type == VAR_QUBIT){
                ret = qir->valref[node->index];
                break;
            }
            ret = LLVMBuildLoad2(qir->builder, get_type(node->resolved_type), qir->valref[node->index], "");
            break;

        case VALUE:
            if(node->resolved_type == VAR_DOUBLE){
                ret = LLVMConstReal(double_type, strtod(node->value, NULL));
                break;
            }
            if(node->resolved_type == VAR_BIT){
                ret = LLVMConstInt(i1_type, strtol(node->value, NULL, 10), 0);
                break;
            }
            ret = LLVMConstInt(i64_type, strtol(node->value, NULL, 10), 1);
            break;
        default:
            break;
    }
    return ret;
}


void generate_declare(qir_context *qir, ast *node){
    if(node->index == -1){
        add_error_entry(INTERNAL, node->line, "Unable to access index of variable");
        return;
    }
    if(node->resolved_type == VAR_QUBIT){
        qir->valref[node->index] = LLVMConstIntToPtr(LLVMConstInt(i64_type, (unsigned long long)ctx->var_list[node->index].qubit, 1), ptr_type);
    } else {
        LLVMBasicBlockRef last_block = LLVMGetInsertBlock(qir->builder);
        LLVMPositionBuilderBefore(qir->builder, LLVMGetBasicBlockTerminator(qir->entry_block));
        qir->valref[node->index] = LLVMBuildAlloca(qir->builder, get_type(node->resolved_type), "");
        LLVMPositionBuilderAtEnd(qir->builder, last_block);
    }

    if(print) printf("NEW VAR %s\n",node->value);
}

void generate_assign(qir_context *qir, ast *node){
    if(node->left->type == NAME){
        generate_declare(qir, node->left);
    }

    if(node->left->resolved_type == VAR_QUBIT){
        if(node->right->resolved_type == VAR_INTEGER && strcmp(node->right->value, "1") == 0){
            LLVMBasicBlockRef last_block = LLVMGetInsertBlock(qir->builder);
            LLVMPositionBuilderBefore(qir->builder, LLVMGetBasicBlockTerminator(qir->entry_block));


            LLVMValueRef args[1] = {qir->valref[node->left->index]};
            LLVMTypeRef param[1] = {ptr_type};
            emit_gate(qir, 1, args, param);

            LLVMPositionBuilderAtEnd(qir->builder, last_block);
        }
        return;
    }

    LLVMValueRef res = generate_expression(qir, node->right);
    if(res == 0) return;

    LLVMBuildStore(qir->builder, res, qir->valref[node->left->index]);
}

void generate_call(qir_context *qir, ast *node){
    if(node->index < NUM_GATES){ // gate call
        gate call_gate = q_gates[node->index];

        LLVMTypeRef param[call_gate.args];
        ast *current_node = node->left;
        for(int i = 0; i < call_gate.args; i++){
            param[i] = get_type(current_node->resolved_type);
            current_node = current_node->branch;
        }

        LLVMValueRef args[call_gate.args];
        current_node = node->left;
        for(int i = 0; i < call_gate.args; i++){
            args[i] = qir->valref[current_node->index];
            current_node = current_node->branch;
        }

        emit_gate(qir, node->index, args, param);
        
    } else { // custom func call
        function fn = ctx->func_list[node->index-NUM_GATES];
        LLVMValueRef args[fn.num_param];
        ast *current_node = node->left;
        for(int i = 0; i < fn.num_param; i++){
            args[i] = generate_expression(qir, current_node);
            current_node = current_node->branch;
        }

        LLVMBuildCall2(qir->builder, qir->func_type[node->index-NUM_GATES], qir->func_fn[node->index-NUM_GATES], args, fn.num_param, "");
    }

    if(print) printf("%s CALL\n", node->value);
}

void generate_measure(qir_context *qir, ast *node){
    qir->result_list[node->res_id] = qir->valref[node->index];
    if(adaptive){
        LLVMValueRef mz_args[2] = { qir->result_list[node->res_id], LLVMConstIntToPtr(LLVMConstInt(i64_type, (unsigned long long)node->res_id, 1), ptr_type)};
        LLVMBuildCall2(qir->builder, measure_type, measure_function, mz_args, 2, "");
    }

    if(print) printf("MEASUREMENT with %s\n",node->value);
}

void generate_if(qir_context *qir, ast *node){
    LLVMBasicBlockRef if_condition_block = LLVMAppendBasicBlockInContext(qir->context, current_function, "if_condition");
    LLVMBasicBlockRef if_body_block = LLVMAppendBasicBlockInContext(qir->context, current_function, "if_body");
    LLVMBasicBlockRef else_body_block = LLVMAppendBasicBlockInContext(qir->context, current_function, "else_body");
    LLVMBasicBlockRef continue_block = LLVMAppendBasicBlockInContext(qir->context, current_function, "continue");

    LLVMBuildBr(qir->builder, if_condition_block);

    LLVMPositionBuilderAtEnd(qir->builder, if_condition_block);
    LLVMValueRef if_condition;
    if(node->left->type == MEASURE){
        generate_measure(qir, node->left);
        LLVMValueRef read_args[1] = { LLVMConstIntToPtr(LLVMConstInt(i64_type, (unsigned long long)node->left->res_id, 1), ptr_type)};
        if_condition = LLVMBuildCall2(qir->builder, read_res_type, read_res_function, read_args, 1, "");
    } else {
        if_condition = generate_expression(qir, node->left);
    }
    LLVMBuildCondBr(qir->builder, if_condition, if_body_block, else_body_block);

    LLVMPositionBuilderAtEnd(qir->builder, if_body_block);
    ast_walk(qir, node->right);
    LLVMBuildBr(qir->builder, continue_block);

    LLVMPositionBuilderAtEnd(qir->builder, else_body_block);
    ast_walk(qir, node->other);
    LLVMBuildBr(qir->builder, continue_block);

    LLVMPositionBuilderAtEnd(qir->builder, continue_block);
    if(print) printf("IF\n");
}

void generate_for(qir_context *qir, ast *node){
    // for loop building blocks
    LLVMBasicBlockRef for_init_block = LLVMAppendBasicBlockInContext(qir->context, current_function, "for_init");
    LLVMBasicBlockRef for_condition_block = LLVMAppendBasicBlockInContext(qir->context, current_function, "for_condition");
    LLVMBasicBlockRef for_body_block = LLVMAppendBasicBlockInContext(qir->context, current_function, "for_body");
    LLVMBasicBlockRef for_exit_block = LLVMAppendBasicBlockInContext(qir->context, current_function, "for_exit");
    
    LLVMBuildBr(qir->builder, for_init_block);

    LLVMPositionBuilderAtEnd(qir->builder, for_init_block);
    generate_statement(qir, node->left); // for intialisations
    LLVMBuildBr(qir->builder, for_condition_block);

    LLVMPositionBuilderAtEnd(qir->builder, for_condition_block);
    LLVMValueRef for_condition = generate_expression(qir, node->other);
    LLVMBuildCondBr(qir->builder, for_condition, for_body_block, for_exit_block);

    LLVMPositionBuilderAtEnd(qir->builder, for_body_block);
    ast_walk(qir, node->right);
    LLVMBuildBr(qir->builder, for_condition_block);


    LLVMPositionBuilderAtEnd(qir->builder, for_exit_block);
    if(print) printf("FOR\n");
}

void generate_while(qir_context *qir, ast *node){
    // while loop building blocks
    LLVMBasicBlockRef while_condition_block = LLVMAppendBasicBlockInContext(qir->context, current_function, "while_condition");
    LLVMBasicBlockRef while_body_block = LLVMAppendBasicBlockInContext(qir->context, current_function, "while_body");
    LLVMBasicBlockRef while_exit_block = LLVMAppendBasicBlockInContext(qir->context, current_function, "while_exit");

    LLVMBuildBr(qir->builder, while_condition_block);

    LLVMPositionBuilderAtEnd(qir->builder, while_condition_block);
    LLVMValueRef while_condition = generate_expression(qir, node->left);
    LLVMBuildCondBr(qir->builder, while_condition, while_body_block, while_exit_block);

    // body
    LLVMPositionBuilderAtEnd(qir->builder, while_body_block);
    ast_walk(qir, node->right);
    LLVMBuildBr(qir->builder, while_condition_block);

    // exit while
    LLVMPositionBuilderAtEnd(qir->builder, while_exit_block);
    if(print) printf("WHILE\n");
}

void generate_function(qir_context *qir, ast *node){
    function fn = ctx->func_list[node->index];
    LLVMTypeRef param[fn.num_param];
    ast *current_node = node->left;
    for(int i = 0; i < fn.num_param; i++){
        param[i] = get_type(current_node->resolved_type);
        current_node = current_node->branch;
    }

    LLVMTypeRef f_type = LLVMFunctionType(get_type(fn.return_type), param, fn.num_param, 0);
    LLVMValueRef f_fn = LLVMAddFunction(qir->module, node->value, f_type);
    LLVMBuilderRef f_builder = LLVMCreateBuilderInContext(qir->context);
    LLVMBuilderRef f_abuilder = LLVMCreateBuilderInContext(qir->context);
    LLVMBasicBlockRef f_entry = LLVMAppendBasicBlockInContext(qir->context, f_fn, "entry");
    LLVMBasicBlockRef f_body = LLVMAppendBasicBlockInContext(qir->context, f_fn, "body");

    qir->func_type[node->index] = f_type;
    qir->func_fn[node->index] = f_fn;

    // save context
    LLVMBasicBlockRef og_entry = qir->entry_block;
    qir->entry_block = f_entry;
    LLVMBasicBlockRef last_block = LLVMGetInsertBlock(qir->builder);

    current_function = f_fn;
    
    LLVMPositionBuilderAtEnd(qir->builder, f_entry);
    
    current_node = node->left;
    for(int i = 0; i < ctx->func_list[node->index].num_param; i++){
        if(current_node->index == -1){
        add_error_entry(INTERNAL, node->line, "Unable to access index of variable");
        return;
        }
        if(current_node->resolved_type == VAR_QUBIT){
            qir->valref[current_node->index] = LLVMGetParam(f_fn, i);
        } else {
            qir->valref[current_node->index] = LLVMBuildAlloca(qir->builder, get_type(current_node->resolved_type), "");
            LLVMBuildStore(qir->builder, LLVMGetParam(f_fn, i), qir->valref[current_node->index]);
        }
        current_node = current_node->branch;
    }

    LLVMBuildBr(qir->builder, f_body);
    LLVMPositionBuilderAtEnd(qir->builder, f_body);

    ast_walk(qir, node->right);

    if(ctx->func_list[node->index].return_type == VAR_VOID){
        LLVMBuildRetVoid(qir->builder);
    } else {
        LLVMBuildRet(qir->builder, LLVMConstInt(get_type(ctx->func_list[node->index].return_type), 0, 0));
    }

    qir->entry_block = og_entry;
    LLVMPositionBuilderAtEnd(qir->builder, last_block);
    current_function = main_function;
    LLVMDisposeBuilder(f_builder);
    LLVMDisposeBuilder(f_abuilder);
}

void generate_return(qir_context *qir, ast *node){
    if(node->left == NULL){
        LLVMBuildRetVoid(qir->builder);
    } else {
        LLVMValueRef res = generate_expression(qir, node->left);
        LLVMBuildRet(qir->builder, res);
    }
}


void generate_statement(qir_context *qir, ast *node){
    switch(node->type){
        case NAME:
            generate_declare(qir, node);
            break;
        case ASSIGN:
            generate_assign(qir, node);
            break;
        case CALL:
            generate_call(qir, node);
            break;
        case MEASURE:
            generate_measure(qir, node);
            break;
        case CONDITIONAL:
            generate_if(qir, node);
            break;
        case FOR_LOOP:
            generate_for(qir, node);
            break;
        case WHILE_LOOP:
            generate_while(qir, node);
            break;
        case FUNCTION:
            generate_function(qir, node);
            break;
        case RETURN:
            generate_return(qir, node);
            break;
        case ROOT:
            generate_statement(qir, node->branch);
            break;
        default:
            add_error_entry(ERROR, node->line, "Expected a statement");
    }
}

void ast_walk(qir_context *qir, ast *node){
    while(node){
        generate_statement(qir, node);
        node = node->branch;
    }
}


FILE *generate_QIR(ast *root){
    // setup for qir_context
    qir_context *qir = calloc(1, sizeof(qir_context));

    // general LLVM setup
    qir->context = LLVMContextCreate();
    qir->module = LLVMModuleCreateWithNameInContext("QUALLE_module", qir->context);
    qir->builder = LLVMCreateBuilderInContext(qir->context);

    i32_type = LLVMInt32TypeInContext(qir->context);
    i64_type = LLVMInt64TypeInContext(qir->context);
    i1_type = LLVMInt1TypeInContext(qir->context);
    void_type = LLVMVoidTypeInContext(qir->context);
    ptr_type = LLVMPointerTypeInContext(qir->context, 0);
    double_type = LLVMDoubleTypeInContext(qir->context);
    LLVMTypeRef one_param[1] = { ptr_type };
    LLVMTypeRef two_param[2] = { ptr_type , ptr_type };


    // analyse the ast
    if(print) printf("ANALYSER PHASE:\n");
    ctx = analyse_ast(root);

    if(print) printf("\n");

    if(print) printf("GENERATOR PHASE:\n");

    // lists for llvm pointers
    qir->valref = calloc(ctx->num_vars, sizeof(LLVMValueRef));
    qir->result_list = calloc(ctx->num_results, sizeof(LLVMValueRef));
    qir->func_type = calloc(ctx->num_funcs, sizeof(LLVMTypeRef));
    qir->func_fn = calloc(ctx->num_funcs, sizeof(LLVMValueRef));
    qir->gate_type = calloc(NUM_GATES, sizeof(LLVMTypeRef));
    qir->gate_fn = calloc(NUM_GATES, sizeof(LLVMValueRef));

    // define basic functions
    // main function
    main_type = LLVMFunctionType(i64_type, NULL, 0, 0);
    main_function = LLVMAddFunction(qir->module, "main", main_type);
    current_function = main_function;
    
    // measure function
    measure_type = LLVMFunctionType(void_type, two_param, 2, 0);
    measure_function = LLVMAddFunction(qir->module, "__quantum__qis__mz__body", measure_type);

    read_res_type = LLVMFunctionType(i1_type, one_param, 1, 0);
    read_res_function = LLVMAddFunction(qir->module, "__quantum__rt__read__result", read_res_type);

    // record output
    LLVMTypeRef result_type = LLVMFunctionType(void_type, two_param, 2, 0);
    LLVMValueRef result_function = LLVMAddFunction(qir->module, "__quantum__rt__result_record_output", result_type);

    // init function
    LLVMTypeRef init_type = LLVMFunctionType(void_type, one_param, 1, 0);
    LLVMValueRef init_function = LLVMAddFunction(qir->module, "__quantum__rt__initialize", init_type);

    
    // build block structure
    qir->entry_block = LLVMAppendBasicBlockInContext(qir->context, main_function, "entry");
    LLVMBasicBlockRef body_block = LLVMAppendBasicBlockInContext(qir->context, main_function, "body");

    // entry block + init function
    LLVMPositionBuilderAtEnd(qir->builder, qir->entry_block);
    LLVMValueRef init_args[1] = { LLVMConstNull(ptr_type) };
    LLVMBuildCall2(qir->builder, init_type, init_function, init_args, 1, "");
    LLVMBuildBr(qir->builder, body_block);

    // body block + generate instructions
    LLVMPositionBuilderAtEnd(qir->builder, body_block);

    if(print) printf("\nGENERATOR:\n");
    ast_walk(qir, root->branch);

    // measure block
    // this block is only used in a base profile program
    if(!adaptive){
        LLVMBasicBlockRef measure_block = LLVMAppendBasicBlockInContext(qir->context, main_function, "measure");
        LLVMBuildBr(qir->builder, measure_block);
        LLVMPositionBuilderAtEnd(qir->builder, measure_block);
    
        for(int i = 0; i < ctx->num_results; i++){
            LLVMValueRef mz_args[2] = { qir->result_list[i], LLVMConstIntToPtr(LLVMConstInt(i64_type, (unsigned long long)i, 1), ptr_type)};
            LLVMBuildCall2(qir->builder, measure_type, measure_function, mz_args, 2, "");
        }
    
    }
    LLVMBasicBlockRef output_block = LLVMAppendBasicBlockInContext(qir->context, main_function, "output");
    LLVMBuildBr(qir->builder, output_block);

    // output block
    LLVMPositionBuilderAtEnd(qir->builder, output_block);
    
    for(int i = 0; i < ctx->num_results; i++){
        LLVMValueRef label = make_label(qir, i);
        LLVMValueRef rs_args[2] = { LLVMConstIntToPtr(LLVMConstInt(i64_type, (unsigned long long)i, 1), ptr_type), label};

        LLVMBuildCall2(qir->builder, result_type, result_function, rs_args, 2, "");
    }
    
    // return 0
    LLVMBuildRet(qir->builder, LLVMConstInt(i64_type, 0, 0));

    // convert the number of qubits and results to strings
    int len_qubits = floor(log10(ctx->num_qubits)) + 1; // this calculates the number of chars
    if(ctx->num_qubits == 0) len_qubits = 1;
    char *str_qubits = malloc(len_qubits+1);
    snprintf(str_qubits, len_qubits+1, "%d", ctx->num_qubits);

    int len_results = floor(log10(ctx->num_results)) + 1;
    if(ctx->num_results == 0) len_results = 1;
    char *str_results = malloc(len_results+1);
    snprintf(str_results, len_results+1, "%d", ctx->num_results);


    // define all attributes
    LLVMAttributeRef entryAttribute = LLVMCreateStringAttribute(qir->context, "entry_point", 11, "", 0);
    LLVMAttributeRef labelingAttribute = LLVMCreateStringAttribute(qir->context, "output_labeling_schema", 22, "", 0);
    LLVMAttributeRef qubitsAttribute = LLVMCreateStringAttribute(qir->context, "required_num_qubits", 19, str_qubits, len_qubits);
    LLVMAttributeRef resultsAttribute = LLVMCreateStringAttribute(qir->context, "required_num_results", 20, str_results, len_results);
    LLVMAttributeRef profileAttribute;
    if(adaptive) profileAttribute = LLVMCreateStringAttribute(qir->context, "qir_profiles", 12, "adaptive_profile", 16);
    else profileAttribute = LLVMCreateStringAttribute(qir->context, "qir_profiles", 12, "base_profile", 12);

    LLVMAddAttributeAtIndex(main_function, LLVMAttributeFunctionIndex, entryAttribute);
    LLVMAddAttributeAtIndex(main_function, LLVMAttributeFunctionIndex, labelingAttribute);
    LLVMAddAttributeAtIndex(main_function, LLVMAttributeFunctionIndex, profileAttribute);
    LLVMAddAttributeAtIndex(main_function, LLVMAttributeFunctionIndex, qubitsAttribute);
    LLVMAddAttributeAtIndex(main_function, LLVMAttributeFunctionIndex, resultsAttribute);

    // measurement attribute
    unsigned kind = LLVMGetEnumAttributeKindForName("writeonly", 9);
    LLVMAttributeRef irrevAttribute = LLVMCreateStringAttribute(qir->context, "irreversible", 12, "", 0);
    LLVMAttributeRef writeonlyAttribute = LLVMCreateEnumAttribute(qir->context, kind, 0);
    
    LLVMAddAttributeAtIndex(measure_function, 2, writeonlyAttribute);
    LLVMAddAttributeAtIndex(measure_function, LLVMAttributeFunctionIndex, irrevAttribute);

    // readonly attribute
    kind = LLVMGetEnumAttributeKindForName("readonly", 8);
    LLVMAttributeRef readonlyAttribute = LLVMCreateEnumAttribute(qir->context, kind, 0);
    
    LLVMAddAttributeAtIndex(result_function, 1, readonlyAttribute);


    // add module flags these are QIR specific
    LLVMMetadataRef meta_major = LLVMValueAsMetadata(LLVMConstInt(i32_type, QIR_MAJOR_VERSION, 0));
    LLVMMetadataRef meta_minor = LLVMValueAsMetadata(LLVMConstInt(i32_type, QIR_MINOR_VERSION, 0));
    LLVMMetadataRef meta_dynamic_qu = LLVMValueAsMetadata(LLVMConstInt(i1_type, adaptive, 0));
    LLVMMetadataRef meta_dynamic_res = LLVMValueAsMetadata(LLVMConstInt(i1_type, adaptive, 0));

    LLVMAddModuleFlag(qir->module, LLVMModuleFlagBehaviorError, "qir_major_version", 17, meta_major);
    LLVMAddModuleFlag(qir->module, 6, "qir_minor_version", 17, meta_minor);
    LLVMAddModuleFlag(qir->module, LLVMModuleFlagBehaviorError, "dynamic_qubit_management", 24, meta_dynamic_qu);
    LLVMAddModuleFlag(qir->module, LLVMModuleFlagBehaviorError, "dynamic_result_management", 25, meta_dynamic_res);

    free(str_qubits);
    free(str_results);
    // release the list

    // string for error handling
    char **error = NULL;
    FILE *output = NULL;

    LLVMVerifyModule(qir->module, LLVMReturnStatusAction, error);
    if(error != NULL){
        add_error_entry(ERROR, -1, "Failed to verify QIR code");

        fprintf(stderr, "VERIFICATION ERROR: \n");
        for(int i = 0; error[i] != NULL; i++){
            fprintf(stderr, "%s\n", error[i]);
        }
        free(error);
    }
    // if bitcode is set, we generate a .bc file
    if(!ll){
        if(LLVMWriteBitcodeToFile(qir->module, "output.bc") != 0){
            add_error_entry(ERROR, -1, "Error while writing .bc file");
            goto dispose;
        }
        output = fopen("output.bc", "r+");
    } else { // if not, we generate a human readable .ll file
        if(LLVMPrintModuleToFile(qir->module, "output.ll", error) != 0){
            add_error_entry(ERROR, -1, "Error while writing .ll file");
            fprintf(stderr, "VERIFICATION ERROR: \n");
            for(int i = 0; error[i] != NULL; i++){
                fprintf(stderr, "%s\n", error[i]);
            }
            free(error);
            goto dispose;
        }
        output = fopen("output.ll", "r+");
    }
    
    if(!output){
        add_error_entry(ERROR, -1, "Unable to open output file");
    }

    dispose:
    free_context(ctx);
    
    free(qir->valref);
    free(qir->result_list);
    free(qir->func_type);
    free(qir->func_fn);
    free(qir->gate_type);
    free(qir->gate_fn);
    
    LLVMDisposeBuilder(qir->builder);
    LLVMDisposeModule(qir->module);
    LLVMContextDispose(qir->context);
    
    free(qir);
    
    check_errors();
    return output;
}   