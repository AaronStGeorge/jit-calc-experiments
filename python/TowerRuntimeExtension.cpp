#include "mlir/Bindings/Python/Diagnostics.h"
#include "mlir/Bindings/Python/NanobindAdaptors.h"

#include "jit-calc-mlir-c/Dialect/TowerRuntime.h"

#include <nanobind/nanobind.h>

using namespace mlir::python;

namespace nb = nanobind;

NB_MODULE(_towerRuntimeDialect, m) {
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
      nb::arg("context") = nb::none(), nb::arg("load") = true);

  //===--------------------------------------------------------------------===//
  // ContextType
  //===--------------------------------------------------------------------===//

  nanobind_adaptors::mlir_type_subclass(towerRuntimeM, "ContextType",
                                        mlirTypeIsATowerRuntimeContextType)
      .def_classmethod(
          "get",
          [](nb::object cls, MlirContext context) {
            CollectDiagnosticsToStringScope scope(context);
            MlirType type = mlirTowerRuntimeContextTypeGet(context);
            if (mlirTypeIsNull(type)) {
              throw nb::value_error(scope.takeMessage().c_str());
            }
            return cls(type);
          },
          nb::arg("cls"), nb::kw_only(), nb::arg("context") = nb::none());
}