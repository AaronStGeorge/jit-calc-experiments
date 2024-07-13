#include "mlir/Conversion/LLVMCommon/MemRefBuilder.h"
#include "mlir/Conversion/LLVMCommon/TypeConverter.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/LLVMIR/LLVMTypes.h"
#include "mlir/Dialect/MemRef/Utils/MemRefUtils.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Types.h"
#include "mlir/IR/Value.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constants.h"
#include <mlir/IR/MLIRContext.h>
#include <mlir/InitAllDialects.h>

using namespace mlir;

int main(int argc, char **argv) {
  mlir::MLIRContext context;
  mlir::registerAllDialects(context);
  context.loadAllAvailableDialects();
  mlir::OpBuilder builder(&context);

  auto loc = builder.getUnknownLoc();

  mlir::ModuleOp theModule = mlir::ModuleOp::create(loc);

  builder.setInsertionPointToEnd(theModule.getBody());

  LLVMTypeConverter typeConverter(&context);

  auto elementType = mlir::FloatType::getF32(&context);
  llvm::ArrayRef<int64_t> shape = {4, 5};
  mlir::MemRefType memRefType = mlir::MemRefType::get(shape, elementType);

  mlir::Type ptrType = LLVM::LLVMPointerType::get(&context);

  auto ptr = builder.create<LLVM::UndefOp>(loc, ptrType);

  auto bla = MemRefDescriptor::fromStaticShape(builder, loc, typeConverter,
                                               memRefType, ptr);
  llvm::SmallVector<mlir::Value, 4> args;
  MemRefDescriptor::unpack(builder, loc, bla, memRefType, args);

  // auto convertedType = typeConverter.convertType(memRefType);

  // auto des = MemRefDescriptor::undef(builder, loc, convertedType);

  theModule.dump();

  return 0;
}