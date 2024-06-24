// RUN: srf2-opt %s \
// RUN: 	-convert-tower-runtime-to-llvm

func.func @func(%0 : !tower_runtime.ctx) -> () {
	return
}
