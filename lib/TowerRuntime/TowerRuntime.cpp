#include "mlir/ExecutionEngine/CRunnerUtils.h"
#include "mlir/ExecutionEngine/RunnerUtils.h"

#include "TowerRuntime/TowerRuntime.h"

#include <iostream>

extern "C" {
void dump(TowerRuntimeContext *s) {
  std::cout << "{main=" << s->main << ", thread=" << s->thread
            << ", count=" << s->count << ", limit=" << s->limit << "}"
            << std::endl;
}

void check_swap(TowerRuntimeContext *s) {
  if (s->count < s->limit) {
    std::cout << "continue" << std::endl;
    return;
  }
  std::cout << "swap" << std::endl;
  swap(s->thread, s->main);
}
} // extern "C"