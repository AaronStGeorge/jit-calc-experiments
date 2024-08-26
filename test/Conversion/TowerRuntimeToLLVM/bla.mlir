// RUN: srf2-opt %s \
// RUN: 	-convert-tower-runtime-to-llvm

func.func private @dump(!llvm.ptr) -> ()
func.func private @check_swap(!llvm.ptr) -> ()
func.func private @swap(!llvm.ptr, !llvm.ptr) -> (i1)

func.func @entry(%arg0 : !tower_runtime.ctx) -> () {
	tower_runtime.dump %arg0 : !tower_runtime.ctx
	%0 = tower_runtime.increment_count %arg0 : !tower_runtime.ctx
	tower_runtime.dump %0 : !tower_runtime.ctx
	tower_runtime.check_swap %0 : !tower_runtime.ctx
	%1 = tower_runtime.increment_count %0 : !tower_runtime.ctx
	tower_runtime.dump %1 : !tower_runtime.ctx
	tower_runtime.check_swap %1 : !tower_runtime.ctx
	return
}
