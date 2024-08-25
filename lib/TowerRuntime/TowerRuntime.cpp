#include "mlir/ExecutionEngine/CRunnerUtils.h"
#include "mlir/ExecutionEngine/RunnerUtils.h"

#include "TowerRuntime/TowerRuntime.h"

#include <iostream>

extern "C" {
void dump(S *s) {
  std::cout << "{main=" << s->main << ", thread=" << s->thread
            << ", count=" << s->count << ", limit=" << s->limit << "}"
            << std::endl;
}

void check_swap(S *s) {
  if (s->count < s->limit) {
    std::cout << "continue" << std::endl;
    return;
  }
  std::cout << "swap" << std::endl;
  swap_to_main(s);
}
} // extern "C"