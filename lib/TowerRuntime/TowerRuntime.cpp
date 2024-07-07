#include "mlir/ExecutionEngine/CRunnerUtils.h"
#include "mlir/ExecutionEngine/RunnerUtils.h"

#include "TowerRuntime/TowerRuntime.h"

#include <iostream>

extern "C" {
void dump(S *s) {
  std::cout << "ding: {from=" << s->from << ", to=" << s->to
            << ", count=" << s->count << "}" << std::endl;
}

void check_swap(S *s) {
  std::cout << "check_swap: {from=" << s->from << ", to=" << s->to
            << ", count=" << s->count << ", limit=" << s->limit << "}"
            << std::endl;
  if (s->count < s->limit) {
    std::cout << "continue" << std::endl;
    return;
  }
  std::cout << "swap" << std::endl;
}

} // extern "C"