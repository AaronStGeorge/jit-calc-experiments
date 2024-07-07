// RUN: mlir-opt %s \
// RUN: 	-convert-scf-to-cf \
// RUN: 	-convert-cf-to-llvm \
// RUN: 	-convert-vector-to-llvm \
// RUN: 	-finalize-memref-to-llvm \
// RUN: 	-convert-arith-to-llvm \
// RUN: 	-convert-func-to-llvm \
// RUN: 	-reconcile-unrealized-casts \
// RUN:		| mlir-cpu-runner \
// RUN:			-e entry \
// RUN:			-entry-point-result=void \
// RUN:			-shared-libs=%mlir_lib_dir/libmlir_c_runner_utils%shlibext \
// RUN:			-shared-libs=%tower_runtime_lib


func.func private @dump(%t: !llvm.ptr) -> ()

func.func @entry() -> () {
	%c420_i64 = arith.constant 420 :i64
	%c69_i64 = arith.constant 69 :i64
	%c0_i64 = arith.constant 0 :i64
	%c1_i64 = arith.constant 1 :i64
	%c2_i64 = arith.constant 2 :i64
	%c1_index = llvm.mlir.constant(1 : index) : i64
	%0 = llvm.mlir.undef : !llvm.struct<(i64, i64, i64)>
	%1 = llvm.insertvalue %c420_i64, %0[0] : !llvm.struct<(i64, i64, i64)>
	%2 = llvm.insertvalue %c69_i64, %1[1] : !llvm.struct<(i64, i64, i64)>
	%3 = llvm.insertvalue %c0_i64, %2[2] : !llvm.struct<(i64, i64, i64)>
	%4 = llvm.alloca %c1_index x !llvm.struct<(i64, i64, i64)> : (i64) -> !llvm.ptr
	llvm.store %3, %4 : !llvm.struct<(i64, i64, i64)>, !llvm.ptr
	func.call @dump(%4) : (!llvm.ptr) -> ()
	%5 = llvm.load %4 : !llvm.ptr -> !llvm.struct<(i64, i64, i64)>
	%6 = llvm.extractvalue %5[2] : !llvm.struct<(i64, i64, i64)>
	%7 = arith.addi %6, %c2_i64 : i64
	%8 = llvm.insertvalue %7, %5[2] : !llvm.struct<(i64, i64, i64)>
	llvm.store %8, %4 : !llvm.struct<(i64, i64, i64)>, !llvm.ptr
	func.call @dump(%4) : (!llvm.ptr) -> ()
	return
}
