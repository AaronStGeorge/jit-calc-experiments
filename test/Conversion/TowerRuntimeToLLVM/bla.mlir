// RUN: srf2-opt %s \
// RUN: 	-convert-tower-runtime-to-llvm

func.func private @dump(%t: !llvm.ptr) -> ()
func.func private @check_swap(%t: !llvm.ptr) -> ()

func.func @entry(%arg0 : !tower_runtime.ctx) -> () {
	tower_runtime.increment_count %arg0 : !tower_runtime.ctx
	tower_runtime.check_swap %arg0 : !tower_runtime.ctx
	tower_runtime.increment_count %arg0 : !tower_runtime.ctx
	tower_runtime.check_swap %arg0 : !tower_runtime.ctx
	return
}
