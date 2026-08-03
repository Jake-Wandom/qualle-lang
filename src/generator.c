#include "generator.h"
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

// we can define measure and result functions only once
// to use them in other functions, we need them to be global
static LLVMTypeRef measure_type;
static LLVMValueRef measure_function;

static LLVMTypeRef result_type;
static LLVMValueRef result_function;

// to avoid redefinitions we store these globally
static LLVMTypeRef ptr_type;
static LLVMTypeRef void_type;
static LLVMTypeRef int_type;

static LLVMValueRef **llvm_list;

// the number of qubits that are declared and returned is counted dynamically
static int required_num_qubits = 2;
static int required_num_results = 2;

void call_function(qir_context qir, ast *node, char *name){
    LLVMTypeRef f_type;
    LLVMValueRef f_call;
    LLVMValueRef *args = NULL;
    int num_args = 0;

    if(strcmp(name, "H") == 0){
        num_args = 1;
        args = calloc(num_args, sizeof(LLVMValueRef));
        args[0] = *(node->llvm);

        LLVMTypeRef h_param[1] = { ptr_type };
        f_type = LLVMFunctionType(void_type, h_param, num_args, 0);
        f_call = LLVMAddFunction(qir.module, "__quantum__qis__h__body", f_type);
    } else if(strcmp(name, "CNOT") == 0){
        num_args = 2;
        args = calloc(num_args, sizeof(LLVMValueRef));
        args[0] = *(node->llvm);
        args[1] = *(node->branch->llvm);

        LLVMTypeRef h_param[2] = { ptr_type , ptr_type };
        f_type = LLVMFunctionType(void_type, h_param, num_args, 0);
        f_call = LLVMAddFunction(qir.module, "__quantum__qis__cnot__body", f_type);
    } else {
        free(args);
        return;
    }
    /*
    LLVMTypeRef init_param[1] = { ptr_type };
    num_args = 1;
    f_type = LLVMFunctionType(void_type, init_param, 1, 0);
    f_call = LLVMAddFunction(qir.module, "__quantum__rt__initialize", f_type);
    */
    
    LLVMBuildCall2(qir.builder, f_type, f_call, args, num_args, "");
    free(args);
}

LLVMValueRef add_value(qir_context qir, enum variable_type type, unsigned long long value){
    switch(type){
        case VAR_QUBIT:
            return LLVMConstPointerNull(ptr_type);
        case VAR_BIT:
            return LLVMConstInt(LLVMInt1TypeInContext(qir.context), value, 0);
        case VAR_INTEGER:
            return LLVMConstInt(LLVMInt32TypeInContext(qir.context), value, 0);
        default:
            return NULL;
    }
}

void generate_instructions(qir_context qir, ast *node){
    if(!node) return;

    switch(node->type){
        case CALL:
            call_function(qir, node->left, node->name);
            break;
        case MEASURE:
            LLVMValueRef result = LLVMConstPointerNull(ptr_type);
            LLVMValueRef variable = *(node->llvm);
            LLVMValueRef mz[2] = { variable, result };
            LLVMBuildCall2(qir.builder, measure_type, measure_function, mz, 2, "");
            LLVMBuildCall2(qir.builder, result_type, result_function, mz, 2, "");
            break;
        case TYPE:
            if(node->branch->type != NAME) return;
            if(node->branch->llvm == NULL) {
                return;
            }
            *(node->branch->llvm) = add_value(qir, node->var_type, 0);
            // TODO llvm_list
            
        case ASSIGN:
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

    LLVMTypeRef i32_type = LLVMInt32TypeInContext(qir.context);
    LLVMTypeRef i64_type = LLVMInt64TypeInContext(qir.context);
    LLVMTypeRef i1_type = LLVMInt1TypeInContext(qir.context);
    void_type = LLVMVoidTypeInContext(qir.context);
    ptr_type = LLVMPointerTypeInContext(qir.context, 0);


    // define basic functions
    // main function
    LLVMTypeRef main_type = LLVMFunctionType(void_type, NULL, 0, 0);
    LLVMValueRef main_function = LLVMAddFunction(qir.module, "main", main_type);
    
    // measure function
    LLVMTypeRef measure_param[2] = {ptr_type, ptr_type};
    measure_type = LLVMFunctionType(void_type, measure_param, 2, 0);
    measure_function = LLVMAddFunction(qir.module, "__quantum__qis__mz__body", measure_type);

    // record output
    LLVMTypeRef result_param[2] = {ptr_type, ptr_type};
    result_type = LLVMFunctionType(void_type, result_param, 2, 0);
    result_function = LLVMAddFunction(qir.module, "__quantum__rt__result_record_output", result_type);

    
    // append start point
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(qir.context, main_function, "entry");
    LLVMPositionBuilderAtEnd(qir.builder, entry);


    int size = analyse_ast(root);
    if(size == -1){
        LLVMDisposeBuilder(qir.builder);
        LLVMDisposeModule(qir.module);
        LLVMContextDispose(qir.context);
        return NULL;
    }

    // create a list for all llvm pointers
    llvm_list = calloc(size, sizeof(LLVMValueRef*));
    
    generate_instructions(qir, root);
    

    //LLVMBuildCall2(qir.builder, result_type, result_function, mz, 2, "");


    // return void
    LLVMBuildRetVoid(qir.builder);

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
    LLVMAttributeRef profileAttribute = LLVMCreateStringAttribute(qir.context, "qir_profile", 11, "base_profile", 12);
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