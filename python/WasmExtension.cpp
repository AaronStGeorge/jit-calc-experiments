#include "jit-calc-mlir-c/Dialect/Wasm.h"
#include "mlir/Bindings/Python/NanobindAdaptors.h"

#include <nanobind/nanobind.h>

using namespace mlir::python;

namespace nb = nanobind;

NB_MODULE(_wasmDialect, m) {
  //===--------------------------------------------------------------------===//
  // Wasm dialect
  //===--------------------------------------------------------------------===//
  auto wasmM = m.def_submodule("mlir_wasm");

  wasmM.def(
      "register_dialect",
      [](MlirContext context, bool load) {
        MlirDialectHandle handle = mlirGetDialectHandle__wasm__();
        mlirDialectHandleRegisterDialect(handle, context);
        if (load) {
          mlirDialectHandleLoadDialect(handle, context);
        }
      },
      nb::arg("context") = nb::none(), nb::arg("load") = true);
}
