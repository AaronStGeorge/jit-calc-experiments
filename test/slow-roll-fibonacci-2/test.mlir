// RUN: mlir-opt %s \
// RUN: 	-convert-scf-to-cf \
// RUN:		-mem2reg \
// RUN: 	-convert-cf-to-llvm \
// RUN: 	-convert-vector-to-llvm \
// RUN: 	-convert-arith-to-llvm \
// RUN: 	-finalize-memref-to-llvm \
// RUN: 	-convert-func-to-llvm \
// RUN: 	-reconcile-unrealized-casts
// 		| mlir-cpu-runner \
// 			-e entry \
// 			-entry-point-result=void \
// 			-shared-libs=%mlir_lib_dir/libmlir_c_runner_utils.so


// int i = 0;
// int j = 1;

// for (int c = 0; c < n; c++) {
//   int tmp = i + j;
//   i = j;
//   j = tmp;
// }
// return i;
func.func @entry() {
	%c0_index	 = arith.constant 0 : index
	%c1_index	 = arith.constant 1 : index

	%c0_i32	 = arith.constant 0 : i32
	%c1_i32	 = arith.constant 1 : i32

	%n = arith.constant 11 : index

	%i = memref.alloca() : memref<i32>
  memref.store %c0_i32, %i[] : memref<i32>

	%j = memref.alloca() : memref<i32>
  memref.store %c1_i32, %j[] : memref<i32>

	scf.for %c = %c0_index to %n step %c1_index {
		%i_load = memref.load %i[] : memref<i32>
		%j_load = memref.load %j[] : memref<i32>
		%tmp = arith.addi %i_load, %j_load : i32
		memref.store %j_load, %i[] : memref<i32>
		memref.store %tmp, %j[] : memref<i32>
	}

	%0 = memref.load %i[] : memref<i32>
  vector.print %0 : i32
  return
}