#include "generator.h"
#include "helper.h"
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
#define ADAPTIVE 0

bool ll = 0;

// to avoid redefinitions we store these globally
static LLVMTypeRef ptr_type;
static LLVMTypeRef void_type;
static LLVMTypeRef i32_type;
static LLVMTypeRef i64_type;
static LLVMTypeRef i1_type;

// the number of qubits that are declared and returned is counted dynamically
static int required_num_qubits = 0;
static int required_num_results = 0;

// dynamic array for measurement and output recording
LLVMValueRef *result_list = NULL;

// global variables for function definitions
static LLVMTypeRef h_type;
static LLVMValueRef h_function;
static LLVMTypeRef cnot_type;
static LLVMValueRef cnot_function;


int find_variable(variable *var_list, size_t size, LLVMValueRef *llvm){
    for(int i = 0; i < size; i++){
        if(var_list[i].llvm == llvm){
            return i;
        }
    }
    return -1;
}

LLVMValueRef make_label(qir_context qir, int number){
    size_t temp_size = floor(log10(number))+2;
    if(number <= 0) temp_size = 2;
        
    char *label_name = calloc(1, temp_size);
    snprintf(label_name, temp_size, "%i", number);
    char *label_identifier = calloc(1, 1+temp_size);
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
    LLVMValueRef *args = NULL;

    if(strcmp(name, "H") == 0){
        args = calloc(1, sizeof(LLVMValueRef));
        args[0] = *(node->llvm);

        LLVMBuildCall2(qir.builder, h_type, h_function, args, 1, "");

        if(print) printf("HADAMARD CALL with %s:%p\n",node->name ,node->llvm);
    } else if(strcmp(name, "CNOT") == 0){
        args = calloc(2, sizeof(LLVMValueRef));
        args[0] = *(node->llvm);
        args[1] = *(node->branch->llvm);

        LLVMBuildCall2(qir.builder, cnot_type, cnot_function, args, 2, "");

        if(print) printf("CNOT CALL with %s:%p    %s:%p\n",node->name ,node->llvm, node->branch->name, node->branch->llvm);
    }

    free(args);
}

int add_value(qir_context qir, enum variable_type type, unsigned long long value, LLVMValueRef *llvm){
    switch(type){
        case VAR_QUBIT:
            *llvm = LLVMConstIntToPtr(LLVMConstInt(i64_type, required_num_qubits, 0), ptr_type);

            if(required_num_qubits == 0){
                //LLVMBuildCall2(qir.builder, init_type, init_function, llvm, 1, "");
            }

            required_num_qubits++;
            break;

        case VAR_BIT:
            *llvm = LLVMConstInt(i1_type, value, 0);
            break;

        case VAR_INTEGER:
            *llvm = LLVMConstInt(i32_type, value, 0);
            break;

        default:
            fprintf(stderr, "Error while generating\n");
            return -1;
    }

    return 0;
}

void generate_instructions(qir_context qir, ast *node){
    if(!node) return;

    switch(node->type){
        case CALL:
            call_function(qir, node->left, node->name);
            break;

        case MEASURE:
            required_num_results++;
            result_list = realloc(result_list, required_num_results*sizeof(LLVMValueRef));

            result_list[required_num_results-1] = *(node->llvm);

            if(print) printf("MEASUREMENT with %s:%p\n",node->name ,node->llvm);
            break;

        case TYPE:
            if(node->branch->type != NAME) return;
            if(node->branch->llvm == NULL) {
                return;
            }
            if(add_value(qir, node->var_type, 0, node->branch->llvm) == -1) return;

            if(print) printf("NEW VAR %s:%p\n",node->branch->name ,node->branch->llvm);
            break;

        case ASSIGN:
            if(add_value(qir, node->left->var_type, strtol(node->right->value, NULL, 10), node->left->branch->llvm) == -1) return;

            if(print) printf("NEW VAR %s:%p\n",node->left->branch->name ,node->left->branch->llvm);
            break;
        case FUNCTION:
            break;
        default:
    }
    generate_instructions(qir, node->branch);
}


FILE *generate_QIR(ast *root){
    // setup for var_array and qir_context
    LLVMValueRef *var_array;
    qir_context qir;
    // general LLVM setup
    qir.context = LLVMContextCreate();
    qir.module = LLVMModuleCreateWithNameInContext("QUALLE_module", qir.context);
    qir.builder = LLVMCreateBuilderInContext(qir.context);

    i32_type = LLVMInt32TypeInContext(qir.context);
    i64_type = LLVMInt64TypeInContext(qir.context);
    i1_type = LLVMInt1TypeInContext(qir.context);
    void_type = LLVMVoidTypeInContext(qir.context);
    ptr_type = LLVMPointerTypeInContext(qir.context, 0);
    LLVMTypeRef one_param[1] = { ptr_type };
    LLVMTypeRef two_param[2] = { ptr_type , ptr_type };


    // analyse the ast
    size_t size = count_nodes(root);
    variable *variable_list = analyse_ast(root);

    
    // define basic functions
    // main function
    LLVMTypeRef main_type = LLVMFunctionType(i64_type, NULL, 0, 0);
    LLVMValueRef main_function = LLVMAddFunction(qir.module, "main", main_type);
    
    // measure function
    LLVMTypeRef measure_type = LLVMFunctionType(void_type, two_param, 2, 0);
    LLVMValueRef measure_function = LLVMAddFunction(qir.module, "__quantum__qis__mz__body", measure_type);

    // record output
    LLVMTypeRef result_type = LLVMFunctionType(void_type, two_param, 2, 0);
    LLVMValueRef result_function = LLVMAddFunction(qir.module, "__quantum__rt__result_record_output", result_type);

    // init function
    LLVMTypeRef init_type = LLVMFunctionType(void_type, one_param, 1, 0);
    LLVMValueRef init_function = LLVMAddFunction(qir.module, "__quantum__rt__initialize", init_type);

    // other functions
    h_type = LLVMFunctionType(void_type, one_param, 1, 0);
    h_function = LLVMAddFunction(qir.module, "__quantum__qis__h__body", h_type);
    cnot_type = LLVMFunctionType(void_type, two_param, 2, 0);
    cnot_function = LLVMAddFunction(qir.module, "__quantum__qis__cnot__body", cnot_type);
    
    
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

    LLVMBuildBr(qir.builder, measure_block);

    // measure block
    LLVMPositionBuilderAtEnd(qir.builder, measure_block);

    for(int i = 0; i < required_num_results; i++){
        LLVMValueRef mz_args[2] = { result_list[i], result_list[i]};
        LLVMBuildCall2(qir.builder, measure_type, measure_function, mz_args, 2, "");
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
    char *str_qubits = malloc(len_qubits+1);
    snprintf(str_qubits, len_qubits+1, "%d", required_num_qubits);

    int  len_results = floor(log10(required_num_results)) + 1; 
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
    LLVMMetadataRef meta_dynamic_qu = LLVMValueAsMetadata(LLVMConstInt(i1_type, ADAPTIVE, 0));
    LLVMMetadataRef meta_dynamic_res = LLVMValueAsMetadata(LLVMConstInt(i1_type, ADAPTIVE, 0));

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
            fprintf(stderr, "Error writing to .bc file\n");
            return NULL;
        }
        output = fopen("output.bc", "r+");
    } else { // if not, we generate a human readable .ll file
        if(LLVMPrintModuleToFile(qir.module, "output.ll", &error) != 0){
            fprintf(stderr, "Error writing to .ll file: %s\n", error);
            LLVMDisposeMessage(error);
            return NULL;
        }
        output = fopen("output.ll", "r+");
    }
    
    if(!output){
        fprintf(stderr, "Unable to open output file\n");
        return NULL;
    }

    LLVMDisposeBuilder(qir.builder);
    LLVMDisposeModule(qir.module);
    LLVMContextDispose(qir.context);
    return output;
}   