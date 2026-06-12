#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <stdio.h>


int main() {
    LLVMContextRef ctx    = LLVMContextCreate();
    LLVMModuleRef  mod    = LLVMModuleCreateWithNameInContext("my_module", ctx);
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(ctx);

    // Define: i32 add(i32 a, i32 b)
    LLVMTypeRef  param_types[] = { LLVMInt32TypeInContext(ctx), LLVMInt32TypeInContext(ctx) };
    LLVMTypeRef  fn_type = LLVMFunctionType(LLVMInt32TypeInContext(ctx), param_types, 2, 0);
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
    LLVMWriteBitcodeToFile(mod, "output.bc");

    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(mod);
    LLVMContextDispose(ctx);
    return 0;
}