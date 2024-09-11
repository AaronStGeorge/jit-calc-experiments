#include "jit-calc-mlir-c/Dialect/TowerRuntime.h"
#include "mlir/Bindings/Python/PybindAdaptors.h"

using namespace mlir::python;
using namespace mlir::python::adaptors;

PYBIND11_MODULE(_towerRuntimeDialect, m) {
  //===--------------------------------------------------------------------===//
  // TowerRuntime dialect
  //===--------------------------------------------------------------------===//
  auto towerRuntimeM = m.def_submodule("tower_runtime");

  towerRuntimeM.def(
      "register_dialect",
      [](MlirContext context, bool load) {
        MlirDialectHandle handle = mlirGetDialectHandle__towerRuntime__();
        mlirDialectHandleRegisterDialect(handle, context);
        if (load) {
          mlirDialectHandleLoadDialect(handle, context);
        }
      },
      py::arg("context") = py::none(), py::arg("load") = true);

  //===--------------------------------------------------------------------===//
  // ContextType
  //===--------------------------------------------------------------------===//

  mlir_type_subclass(towerRuntimeM, "ContextType",
                     mlirTypeIsATowerRuntimeContextType)
      .def_classmethod(
          "get",
          [](py::object cls, MlirContext context) {
            CollectDiagnosticsToStringScope scope(context);
            MlirType type = mlirTowerRuntimeContextTypeGet(context);
            if (mlirTypeIsNull(type)) {
              throw py::value_error(scope.takeMessage());
            }
            return cls(type);
          },
          "cls"_a, py::kw_only(), "context"_a = py::none());
}