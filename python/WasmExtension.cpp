#include "Wasm-c/Dialects.h"
#include "mlir/Bindings/Python/PybindAdaptors.h"

using namespace mlir::python::adaptors;

PYBIND11_MODULE(_wasmDialect, m) {
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
      py::arg("context") = py::none(), py::arg("load") = true);
}
