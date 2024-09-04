//===- StandaloneTypes.td - Standalone dialect types -------*- tablegen -*-===//
//
// This file is licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TOWERRUNTIME_TYPES
#define TOWERRUNTIME_TYPES

include "mlir/IR/AttrTypeBase.td"
include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeDialect.td"

//===----------------------------------------------------------------------===//
// TowerRuntime type definitions
//===----------------------------------------------------------------------===//

class TowerRuntime_Type<string name, string typeMnemonic, list<Trait> traits = []>
    : TypeDef<TowerRuntime_Dialect, name, traits> {
  let mnemonic = typeMnemonic;
}

def TowerRuntime_Context : TowerRuntime_Type<"Context", "ctx"> {
    let summary = "TODO";
    let description = "TODO";
}

#endif // TOWERRUNTIME_TYPES
