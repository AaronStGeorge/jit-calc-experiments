// RUN: srf2-opt %s \
// RUN: 	-convert-tower-runtime-to-llvm

func.func private @dump(%t: !llvm.ptr) -> ()

func.func @entry(%arg0 : !tower_runtime.ctx) -> () {
	tower_runtime.dump %arg0 : !tower_runtime.ctx
	%0 = tower_runtime.increment_count %arg0 : !tower_runtime.ctx
	tower_runtime.dump %0 : !tower_runtime.ctx
	return
}
