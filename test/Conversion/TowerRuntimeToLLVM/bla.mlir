// RUN: srf2-opt %s \
// RUN: 	-convert-tower-runtime-to-llvm

func.func private @dump(%t: !llvm.ptr) -> ()
func.func private @check_swap(%t: !llvm.ptr) -> ()

func.func @entry(%arg0 : !tower_runtime.ctx) -> () {
	%0 = tower_runtime.increment_count %arg0 : !tower_runtime.ctx
	tower_runtime.check_swap %0 : !tower_runtime.ctx
	%1 = tower_runtime.increment_count %0 : !tower_runtime.ctx
	tower_runtime.check_swap %1 : !tower_runtime.ctx
	return
}
