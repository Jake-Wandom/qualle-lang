#include "generator.h"
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define QIR_MAJOR_VERSION 2
#define QIR_MINOR_VERSION 0
#define ADAPTIVE 0

LLVMValueRef add_var(LLVMContextRef context, enum variable_type type, unsigned long long value){
    LLVMValueRef var;
    switch(type){
        case VAR_QBIT:
            
            break;
        case VAR_BIT:
            
            break;
        case VAR_INTEGER:
            
            break;
        default:
    }
    return var;
}

void call_function(qir_context qir, LLVMValueRef *args, enum quantum_functions f){
    LLVMTypeRef void_type = LLVMVoidTypeInContext(qir.context);
    LLVMTypeRef ptr_type = LLVMPointerTypeInContext(qir.context, 0);
    LLVMTypeRef f_type;
    LLVMValueRef f_call;
    int num_args = 0;

    switch (f){
        case HADAMARD:
            LLVMTypeRef h_param[1] = { ptr_type };
            num_args = 1;
            f_type = LLVMFunctionType(void_type, h_param, 1, 0);
            f_call = LLVMAddFunction(qir.module, "__quantum__qis__h__body", f_type);

            break;
        default:
            fprintf(stderr, "UNKOWN QUANTUM FUNCTION!\n");
    }
    LLVMBuildCall2(qir.builder, f_type, f_call, args, num_args, "");
}

LLVMValueRef *vari = NULL;

int walk_tree(qir_context qir, ast *node){
    if(node == NULL){
        return 0;
    }

    LLVMTypeRef void_type = LLVMVoidTypeInContext(qir.context);
    LLVMTypeRef ptr_type = LLVMPointerTypeInContext(qir.context, 0);

    switch(node->type){
        case ASSIGN:
            
            break;
        case NAME:
            
            break;
        default:
    }
    walk_tree(qir, node->branch);
    return 0;
}

FILE *generate_QIR(bool bitcode, ast *root){
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
    LLVMTypeRef void_type = LLVMVoidTypeInContext(qir.context);
    LLVMTypeRef ptr_type = LLVMPointerTypeInContext(qir.context, 0);


    // define basic functions
    // init function
    LLVMTypeRef init_param[1] = { ptr_type };
    LLVMTypeRef init_type = LLVMFunctionType(void_type, init_param, 1, 0);
    LLVMValueRef init_function = LLVMAddFunction(qir.module, "__quantum__rt__initialize", init_type);

    // main function
    LLVMTypeRef main_type = LLVMFunctionType(void_type, NULL, 0, 0);
    LLVMValueRef main_function = LLVMAddFunction(qir.module, "main", main_type);

    // measure function
    LLVMTypeRef measure_param[2] = {ptr_type, ptr_type};
    LLVMTypeRef measure_type = LLVMFunctionType(void_type, measure_param, 2, 0);
    LLVMValueRef measure_function = LLVMAddFunction(qir.module, "__quantum__qis__mz__body", measure_type);

    // record output
    LLVMTypeRef result_param[2] = {ptr_type, ptr_type};
    LLVMTypeRef result_type = LLVMFunctionType(void_type, result_param, 2, 0);
    LLVMValueRef result_function = LLVMAddFunction(qir.module, "__quantum__rt__result_record_output", result_type);

    
    // append start point
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(qir.context, main_function, "entry");
    LLVMPositionBuilderAtEnd(qir.builder, entry);

    // initialize qubits
    
    // TODO
    
    /*
    int ret = walk_tree(context, root);
    if(ret == -1){
        fprintf(stderr, "error while generating\n");
        return NULL;
    }
    */

    LLVMValueRef var = LLVMConstPointerNull(ptr_type);
    LLVMValueRef res = LLVMConstPointerNull(ptr_type);
    LLVMValueRef mz[2] = {var, res};

    call_function(qir, &var, HADAMARD);

    LLVMBuildCall2(qir.builder, measure_type, measure_function, mz, 2, "");

    LLVMBuildCall2(qir.builder, result_type, result_function, mz, 2, "");


    // return void
    LLVMBuildRetVoid(qir.builder);

    // define all attributes
    LLVMAttributeRef entryAttribute = LLVMCreateStringAttribute(qir.context, "entry_point", 11, "", 0);
    LLVMAttributeRef labelingAttribute = LLVMCreateStringAttribute(qir.context, "output_labeling_schema", 22, "", 0);
    LLVMAttributeRef profileAttribute = LLVMCreateStringAttribute(qir.context, "qir_profile", 11, "base_profile", 12);
    LLVMAttributeRef qubitsAttribute = LLVMCreateStringAttribute(qir.context, "required_num_qubits", 19, "1", 1);
    LLVMAttributeRef resultsAttribute = LLVMCreateStringAttribute(qir.context, "required_num_results", 20, "1", 1);

    LLVMAddAttributeAtIndex(main_function, LLVMAttributeFunctionIndex, entryAttribute);
    LLVMAddAttributeAtIndex(main_function, LLVMAttributeFunctionIndex, labelingAttribute);
    LLVMAddAttributeAtIndex(main_function, LLVMAttributeFunctionIndex, profileAttribute);
    LLVMAddAttributeAtIndex(main_function, LLVMAttributeFunctionIndex, qubitsAttribute);
    LLVMAddAttributeAtIndex(main_function, LLVMAttributeFunctionIndex, resultsAttribute);

    // measurement attribute
    LLVMAttributeRef irrevAttribute = LLVMCreateStringAttribute(qir.context, "irreversible", 12, "", 0);

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


    // string for error handling
    char *error = NULL;
    FILE *output = NULL;
    // if bitcode is set, we generate a .bc file
    if(bitcode){
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
    free(vari);
    LLVMDisposeBuilder(qir.builder);
    LLVMDisposeModule(qir.module);
    LLVMContextDispose(qir.context);
    return output;
}   