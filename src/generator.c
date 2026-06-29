#include "generator.h"
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <string.h>
#include <stdlib.h>

#define QIR_MAJOR_VERSION 2
#define QIR_MINOR_VERSION 0
#define ADAPTIVE 0

LLVMValueRef add_var(LLVMContextRef context, enum variable_type type, unsigned long long value){
    LLVMTypeRef ptr_type = LLVMPointerTypeInContext(context, 64);
    LLVMTypeRef i1_type = LLVMInt1TypeInContext(context);
    LLVMTypeRef i64_type = LLVMInt64TypeInContext(context);
    
    LLVMValueRef var;
    switch(type){
        case VAR_QBIT:
            var = LLVMConstInt(ptr_type, value, 0);
            break;
        case VAR_BIT:
            var = LLVMConstInt(i1_type, value, 0);
            break;
        case VAR_INTEGER:
            var = LLVMConstInt(i64_type, value, 0);
            break;
        default:
    }
    return var;
}

int walk_tree(LLVMContextRef context, ast *node){
    switch(node->type){
        case ASSIGN:
            if((node->assignment.assignee == TYPE) && (node->assignment.assignor->branch == NUMBER)){
                LLVMValueRef var = add_var(context, node->var_type, strtol(node->assignment.assignor->branch->value, NULL, 10));
            }
            break;
        case INDICATOR:
            if(strcmp(node->value, "H") == 0){
                LLVMTypeRef
            }
        default:
    }
    walk_tree(context, node->branch);
    return 0;
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
    LLVMTypeRef ptr_type = LLVMPointerTypeInContext(context, 64);


    // define main function
    LLVMTypeRef main_type = LLVMFunctionType(void_type, NULL, 0, 0);
    LLVMValueRef main_function = LLVMAddFunction(module, "main", main_type);
 
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(context, main_function, "entry");
    
    LLVMPositionBuilderAtEnd(builder, entry);
    /*
    int ret = walk_tree(context, root);
    if(ret = -1){
        fprintf(stderr, "error while generating");
        return NULL;
    }*/

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
    //LLVMAttributeRef irrevAttribute = LLVMCreateStringAttribute(context, "irreversible", 12, "", 0);

    //LLVMAddAttributeAtIndex(qis_mz, LLVMAttributeFunctionIndex, irrevAttribute);


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