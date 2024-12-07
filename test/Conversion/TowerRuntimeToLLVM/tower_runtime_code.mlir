// RUN: jit-calc-opt %s -convert-tower-runtime-to-llvm | FileCheck %s

// CHECK: func.func @entry(%[[TRCTX0:.*]]: !llvm.struct<(ptr, ptr, i64, i64)>) {
// CHECK:   %[[ALLOCA:.*]] = llvm.alloca %{{.*}} x !llvm.struct<(ptr, ptr, i64, i64)> : (i64) -> !llvm.ptr
// CHECK:   llvm.store %[[TRCTX0]], %[[ALLOCA]] : !llvm.struct<(ptr, ptr, i64, i64)>, !llvm.ptr
// CHECK:   call @dump(%[[ALLOCA]]) : (!llvm.ptr) -> ()
// CHECK:   %[[COUNT:.*]] = llvm.extractvalue %[[TRCTX0]][2] : !llvm.struct<(ptr, ptr, i64, i64)>
// CHECK:   %[[NEW_COUNT:.*]] = arith.addi %{{.*}}, %[[COUNT]] : i64
// CHECK:   %[[TRCTX1:.*]] = llvm.insertvalue %[[NEW_COUNT]], %[[TRCTX0]][2] : !llvm.struct<(ptr, ptr, i64, i64)>
// CHECK:   %[[NEW_COUNT_AGAIN:.*]] = llvm.extractvalue %[[TRCTX1]][2] : !llvm.struct<(ptr, ptr, i64, i64)>
// CHECK:   %[[LIMIT:.*]] = llvm.extractvalue %[[TRCTX1]][3] : !llvm.struct<(ptr, ptr, i64, i64)>
// CHECK:   %[[OVER_LIMIT:.*]] = arith.cmpi uge, %[[NEW_COUNT_AGAIN]], %[[LIMIT]] : i64
// CHECK:   %{{.*}} = scf.if %[[OVER_LIMIT]] -> (!llvm.struct<(ptr, ptr, i64, i64)>) {
// CHECK:     %[[THREAD_CTX:.*]] = llvm.extractvalue %[[TRCTX1]][1] : !llvm.struct<(ptr, ptr, i64, i64)>
// CHECK:     %[[MAIN_CTX:.*]] = llvm.extractvalue %[[TRCTX1]][0] : !llvm.struct<(ptr, ptr, i64, i64)>
// CHECK:     %11 = func.call @swap(%[[THREAD_CTX]], %[[MAIN_CTX]]) : (!llvm.ptr, !llvm.ptr) -> i1
// CHECK:     %[[C0:.*]] = arith.constant 0 : i64
// CHECK:     %[[RTCTX2:.*]] = llvm.insertvalue %[[C0]], %[[TRCTX1]][2] : !llvm.struct<(ptr, ptr, i64, i64)>
// CHECK:     scf.yield %[[RTCTX2]] : !llvm.struct<(ptr, ptr, i64, i64)>
// CHECK:   } else {
// CHECK:     scf.yield %[[TRCTX1]] : !llvm.struct<(ptr, ptr, i64, i64)>
// CHECK:   }
// CHECK:   return
// CHECK: }
func.func @entry(%arg0 : !tower_runtime.ctx) -> () {
	tower_runtime.dump %arg0 : !tower_runtime.ctx
	%0 = tower_runtime.increment_count %arg0 : !tower_runtime.ctx
	tower_runtime.check_swap %0 : !tower_runtime.ctx
	return
}
