// RUN: mlir-opt %s \
// RUN: 	-convert-scf-to-cf \
// RUN: 	-convert-cf-to-llvm \
// RUN: 	-convert-vector-to-llvm \
// RUN: 	-convert-arith-to-llvm \
// RUN: 	-finalize-memref-to-llvm \
// RUN: 	-convert-func-to-llvm \
// RUN: 	-reconcile-unrealized-casts \
// RUN:		| mlir-cpu-runner \
// RUN:			-e entry \
// RUN:			-entry-point-result=void \
// RUN:			-shared-libs=%mlir_lib_dir/libmlir_c_runner_utils.so

func.func @entry() {
	%c0_index	 = arith.constant 0 : index
	%c1_index	 = arith.constant 1 : index
	%c1_i32	 = arith.constant 1 : i32
	%n = arith.constant 9 : index
	%t = memref.alloca() : memref<i32>
  memref.store %c1_i32, %t[] : memref<i32>

	scf.for %c = %c1_index to %n step %c1_index {
		%0 = memref.load %t[] : memref<i32>
  	%1 = arith.index_cast %c : index to i32
		%2 = arith.addi %1, %0 : i32
		memref.store %2, %t[] : memref<i32>
	}

	%0 = memref.load %t[] : memref<i32>
  vector.print %0 : i32
  return
}