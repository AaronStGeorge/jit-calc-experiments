//===- mlir-lsp-server.cpp - MLIR Language Server -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Wasm/WasmDialect.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/InitAllDialects.h"
#include "mlir/InitAllExtensions.h"
#include "mlir/Tools/mlir-lsp-server/MlirLspServerMain.h"

using namespace mlir;

namespace test {
void registerTestDialect(DialectRegistry &);
void registerTestDynDialect(DialectRegistry &);
void registerTestTransformDialectExtension(DialectRegistry &);
} // namespace test

int main(int argc, char **argv) {
  DialectRegistry registry;
  registry.insert<mlir::wasm::WasmDialect>();
  registerAllDialects(registry);
  registerAllExtensions(registry);

  ::test::registerTestDialect(registry);
  ::test::registerTestTransformDialectExtension(registry);
  ::test::registerTestDynDialect(registry);
  return failed(MlirLspServerMain(argc, argv, registry));
}
