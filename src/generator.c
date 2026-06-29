#include "generator.h"
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>


FILE *generate_llvm(){

}

FILE *generate_bitcode(ast *root){
    LLVMContextRef ctx    = LLVMContextCreate();
    LLVMModuleRef  mod    = LLVMModuleCreateWithNameInContext("Module", ctx);
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(ctx);

    // Define: i32 add(i32 a, i32 b)
    LLVMTypeRef  param_types[] = { LLVMInt32TypeInContext(ctx), LLVMInt32TypeInContext(ctx) };
    LLVMTypeRef  fn_type = LLVMFunctionType(LLVMVoidTypeInContext(ctx), param_types, 2, 0);
    LLVMValueRef fn      = LLVMAddFunction(mod, "add", fn_type);

    // Create entry basic block
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(ctx, fn, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    // Build: return a + b
    LLVMValueRef a   = LLVMGetParam(fn, 0);
    LLVMValueRef b   = LLVMGetParam(fn, 1);
    LLVMValueRef sum = LLVMBuildAdd(builder, a, b, "sum");
    LLVMBuildRet(builder, sum);

    // Verify and dump IR
    LLVMVerifyModule(mod, LLVMAbortProcessAction, NULL);
    LLVMDumpModule(mod);  // prints IR to stderr

    // Write bitcode
    char *error;
    LLVMPrintModuleToFile(mod, "output.ll", error);
    LLVMWriteBitcodeToFile(mod, "output.bc");

    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(mod);
    LLVMContextDispose(ctx);
    return 0;
}   