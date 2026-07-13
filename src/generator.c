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
        case INIT:
            LLVMTypeRef init_param[1] = { ptr_type };
            num_args = 1;
            f_type = LLVMFunctionType(void_type, init_param, 1, 0);
            f_call = LLVMAddFunction(qir.module, "__quantum__rt__initialize", f_type);
            break;
        default:
            fprintf(stderr, "UNKOWN QUANTUM FUNCTION!\n");
    }
    LLVMBuildCall2(qir.builder, f_type, f_call, args, num_args, "");
}

LLVMValueRef add_var(qir_context qir, enum variable_type type, unsigned long long value){
    switch(type){
        case VAR_QBIT:
            return LLVMConstPointerNull(LLVMPointerTypeInContext(qir.context,0));
        case VAR_BIT:
            return LLVMConstInt(LLVMInt1TypeInContext(qir.context), value, 0);
        case VAR_INTEGER:
            return LLVMConstInt(LLVMInt32TypeInContext(qir.context), value, 0);
        default:
            return NULL;
    }
}

void generate_instructions(qir_context qir, instruction *start){
    while(start != NULL){
        switch(start->type){
            case DEFINE_VAR:
                printf("define var %s\n", start->var->name);
                start->var->llvm = add_var(qir, start->var->type, start->var->value);
                break;
            case CALL_Q_FUNC:
                printf("call qfunc %s\n", start->func.parameter[0]->name);
                LLVMValueRef *args = malloc(sizeof(LLVMValueRef) * start->func.num_of_param);
                for(int i = 0; i < start->func.num_of_param; i++){
                    args[i] = start->func.parameter[i]->llvm;
                }
                call_function(qir, args, start->q_func);
                break;
            default:
            
        }
        start = start->next_instr;
    }
}

FILE *generate_QIR(bool bitcode, instruction *start){
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

    
    generate_instructions(qir, start);
    LLVMValueRef var = LLVMConstPointerNull(ptr_type);
    LLVMValueRef res = LLVMConstPointerNull(ptr_type);
    LLVMValueRef mz[2] = {var, res};

    call_function(qir, &var, INIT);
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
    LLVMDisposeBuilder(qir.builder);
    LLVMDisposeModule(qir.module);
    LLVMContextDispose(qir.context);
    return output;
}   