#include "generator.h"
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <stdio.h>
#include <stdlib.h>

#define QIR_MAJOR_VERSION 2
#define QIR_MINOR_VERSION 0
#define ADAPTIVE 0

LLVMValueRef add_var(LLVMContextRef context, ast *node){
    if(node->type != VAR_DEF){
        fprintf(stderr, "Wrong ast type for add_var");
        return 0;
    }
    
    LLVMValueRef var;
    switch(node->var.type){
        case VAR_QBIT:
            var = LLVMConstInt(LLVMPointerTypeInContext(context, 0), 0, 0);
            break;
        case VAR_BIT:
            var = LLVMConstInt(LLVMInt1TypeInContext(context), 0, 0);
            break;
        default:
    }
    return var;
}

FILE *generate_QIR(bool bitcode, ast *root){
    // setupt var array
    LLVMValueRef *var_array;
    // general LLVM setup
    LLVMContextRef context = LLVMContextCreate();
    LLVMModuleRef module = LLVMModuleCreateWithNameInContext("QUALLE_module", context);
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);

    LLVMTypeRef i32_type = LLVMInt32TypeInContext(context);
    LLVMTypeRef i64_type = LLVMInt64TypeInContext(context);
    LLVMTypeRef i1_type = LLVMInt1TypeInContext(context);
    LLVMTypeRef void_type = LLVMVoidTypeInContext(context);
    LLVMTypeRef ptr_type = LLVMPointerTypeInContext(context, 0);


    // define main function
    LLVMTypeRef main_type = LLVMFunctionType(void_type, NULL, 0, 0);
    LLVMValueRef main_function = LLVMAddFunction(module, "main", main_type);
 
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(context, main_function, "entry");
    
    LLVMPositionBuilderAtEnd(builder, entry);

    // function definitions
    LLVMTypeRef qis_h_param[1] = { ptr_type };
    LLVMTypeRef qis_h_type = LLVMFunctionType(void_type, qis_h_param, 1, 0);
    LLVMValueRef qis_h = LLVMAddFunction(module, "__quantum__qis__h__body", qis_h_type);

    LLVMTypeRef qis_cnot_param[2] = { ptr_type, ptr_type };
    LLVMTypeRef qis_cnot_type = LLVMFunctionType(void_type, qis_cnot_param, 2, 0);
    LLVMValueRef qis_cnot = LLVMAddFunction(module, "__quantum__qis__cnot__body", qis_cnot_type);

    LLVMTypeRef qis_mz_param[2] = { ptr_type, ptr_type };
    LLVMTypeRef qis_mz_type = LLVMFunctionType(void_type, qis_mz_param, 2, 0);
    LLVMValueRef qis_mz = LLVMAddFunction(module, "__quantum__qis__mz__body", qis_mz_type);

    LLVMValueRef q0 = add_var(context, root->branch);
    LLVMValueRef q1 = add_var(context, root->branch->branch);


    LLVMBuildCall2(builder, qis_h_type, qis_h, &q0, 1, "");
    /*
    LLVMValueRef *qs = malloc(sizeof(LLVMValueRef)*2);
    qs[0] = q0;
    qs[1] = q1; 

    LLVMBuildCall2(builder, qis_cnot_type, qis_cnot, qs, 2, "");

    LLVMBuildCall2(builder, qis_mz_type, qis_mz, qs, 2, "");

    free(qs);
    */


    // define all attributes
    LLVMAttributeRef entryAttribute = LLVMCreateStringAttribute(context, "entry_point", 11, "", 0);
    LLVMAttributeRef labelingAttribute = LLVMCreateStringAttribute(context, "output_labeling_schema", 22, "", 0);
    LLVMAttributeRef profileAttribute = LLVMCreateStringAttribute(context, "qir_profile", 11, "base_profile", 12);
    LLVMAttributeRef qubitsAttribute = LLVMCreateStringAttribute(context, "required_num_qubits", 19, "2", 1);
    LLVMAttributeRef resultsAttribute = LLVMCreateStringAttribute(context, "required_num_results", 20, "2", 1);

    LLVMAddAttributeAtIndex(main_function, LLVMAttributeFunctionIndex, entryAttribute);
    LLVMAddAttributeAtIndex(main_function, LLVMAttributeFunctionIndex, labelingAttribute);
    LLVMAddAttributeAtIndex(main_function, LLVMAttributeFunctionIndex, profileAttribute);
    LLVMAddAttributeAtIndex(main_function, LLVMAttributeFunctionIndex, qubitsAttribute);
    LLVMAddAttributeAtIndex(main_function, LLVMAttributeFunctionIndex, resultsAttribute);

    // measurement attribute
    LLVMAttributeRef irrevAttribute = LLVMCreateStringAttribute(context, "irreversible", 12, "", 0);

    LLVMAddAttributeAtIndex(qis_mz, LLVMAttributeFunctionIndex, irrevAttribute);


    // add module flags these are QIR specific
    LLVMMetadataRef meta_major = LLVMValueAsMetadata(LLVMConstInt(i32_type, QIR_MAJOR_VERSION, 0));
    LLVMMetadataRef meta_minor = LLVMValueAsMetadata(LLVMConstInt(i32_type, QIR_MINOR_VERSION, 0));
    LLVMMetadataRef meta_dynamic_qu = LLVMValueAsMetadata(LLVMConstInt(i1_type, ADAPTIVE, 0));
    LLVMMetadataRef meta_dynamic_res = LLVMValueAsMetadata(LLVMConstInt(i1_type, ADAPTIVE, 0));

    LLVMAddModuleFlag(module, LLVMModuleFlagBehaviorError, "qir_major_version", 17, meta_major);
    LLVMAddModuleFlag(module, LLVMModuleFlagBehaviorAppendUnique, "qir_minor_version", 17, meta_minor);
    LLVMAddModuleFlag(module, LLVMModuleFlagBehaviorError, "dynamic_qubit_management", 24, meta_dynamic_qu);
    LLVMAddModuleFlag(module, LLVMModuleFlagBehaviorError, "dynamic_result_management", 25, meta_dynamic_res);


    // string for error handling
    char *error = NULL;
    FILE *output = NULL;
    // if bitcode is set, we generate a .bc file
    if(bitcode){
        if(LLVMWriteBitcodeToFile(module, "output.bc") != 0){
            fprintf(stderr, "Error writing to .bc file\n");
            return NULL;
        }
        output = fopen("output.bc", "r+");
    } else { // if not, we generate a human readable .ll file
        if(LLVMPrintModuleToFile(module, "output.ll", &error) != 0){
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
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
    LLVMContextDispose(context);
    return output;
}   