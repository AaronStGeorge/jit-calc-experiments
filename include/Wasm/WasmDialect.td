//===- StandaloneDialect.td - Standalone dialect -----------*- tablegen -*-===//
//
// This file is licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef STANDALONE_DIALECT
#define STANDALONE_DIALECT

include "mlir/IR/OpBase.td"

//===----------------------------------------------------------------------===//
// Standalone dialect definition.
//===----------------------------------------------------------------------===//

def Wasm_Dialect : Dialect {
    let name = "wasm";
    let summary = "An MLIR representation for web assembly";
    let description = [{
        TODO
    }];
    let cppNamespace = "::mlir::wasm";
}

//===----------------------------------------------------------------------===//
// Base standalone operation definition.
//===----------------------------------------------------------------------===//

class Wasm_Op<string mnemonic, list<Trait> traits = []> :
        Op<Wasm_Dialect, mnemonic, traits>;

#endif // STANDALONE_DIALECT
