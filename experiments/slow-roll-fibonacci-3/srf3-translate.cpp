#include "mlir/Dialect/WasmSSA/IR/WasmSSA.h"
#include "mlir/IR/AsmState.h"
#include "mlir/IR/DialectRegistry.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/OwningOpRef.h"
#include "mlir/InitAllDialects.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Support/FileUtilities.h"
#include "mlir/Target/Wasm/WasmImporter.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace llvm;

int main(int argc, char **argv) {
  // Set up command line options
  static cl::opt<std::string> inputFilename(
      cl::Positional, cl::desc("<input wasm file>"), cl::init("-"));
  static cl::opt<std::string> outputFilename("o", cl::desc("Output filename"),
                                             cl::value_desc("filename"),
                                             cl::init("-"));

  // Parse command line options
  cl::ParseCommandLineOptions(argc, argv, "WebAssembly to MLIR Translator");

  // Open input file
  std::string errorMessage;
  std::unique_ptr<MemoryBuffer> input =
      openInputFile(inputFilename, &errorMessage);
  if (!input) {
    errs() << "Error opening input file: " << errorMessage << "\n";
    return 1;
  }

  // Ensure null-termination, this might not be necessary, but better safe than
  // sorry.
  if (!input->getBuffer().ends_with('\0')) {
    input = MemoryBuffer::getMemBufferCopy(input->getBuffer(),
                                           input->getBufferIdentifier());
  }

  // Open output file
  auto output = openOutputFile(outputFilename, &errorMessage);
  if (!output) {
    errs() << "Error opening output file: " << errorMessage << "\n";
    return 1;
  }

  // Create MLIR context
  MLIRContext context;

  // Register dialects
  DialectRegistry registry;
  registerAllDialects(registry);
  context.appendDialectRegistry(registry);
  context.loadAllAvailableDialects();

  // Create SourceMgr for the input
  auto sourceMgr = std::make_shared<SourceMgr>();
  sourceMgr->AddNewSourceBuffer(std::move(input), SMLoc());

  // Set up diagnostic handler on the MLIR context. This connects source
  // locations to emitted diagnostics, allowing MLIR to report errors with
  // precise file/line/column information. When MLIR operations emit
  // diagnostics, the handler will map their Location attributes back to
  // positions in the original source files.
  SourceMgrDiagnosticHandler sourceMgrHandler(*sourceMgr, &context);

  // Import WASM module to MLIR
  OwningOpRef<ModuleOp> module =
      wasm::importWebAssemblyToModule(*sourceMgr, &context);

  if (!module) {
    errs() << "Failed to import WebAssembly module\n";
    return 1;
  }

  // Write the MLIR module to output
  module->print(output->os());
  output->os() << '\n';

  // Keep the output file (don't delete it on destruction)
  output->keep();

  return 0;
}
