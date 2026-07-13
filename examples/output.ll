; ModuleID = 'QUALLE_module'
source_filename = "QUALLE_module"

declare void @__quantum__rt__initialize(ptr)

define void @main() #0 {
entry:
  call void @__quantum__qis__h__body(ptr null)
  call void @__quantum__qis__mz__body(ptr null, ptr null)
  call void @__quantum__rt__result_record_output(ptr null, ptr null)
  ret void
}

declare void @__quantum__qis__mz__body(ptr, ptr) #1

declare void @__quantum__rt__result_record_output(ptr, ptr)

declare void @__quantum__qis__h__body(ptr)

attributes #0 = { "entry_point" "output_labeling_schema" "qir_profile"="base_profile" "required_num_qubits"="1" "required_num_results"="1" }
attributes #1 = { "irreversible" }

!llvm.module.flags = !{!0, !1, !2, !3}

!0 = !{i32 1, !"qir_major_version", i32 2}
!1 = !{i32 7, !"qir_minor_version", i32 0}
!2 = !{i32 1, !"dynamic_qubit_management", i1 false}
!3 = !{i32 1, !"dynamic_result_management", i1 false}
