#include "mlir/ExecutionEngine/CRunnerUtils.h"
#include "mlir/ExecutionEngine/RunnerUtils.h"

#include "TowerRuntime/TowerRuntime.h"

#include <iostream>

extern "C" {
void dump(S *s) {
  std::cout << "ding: {from=" << s->from << ", to=" << s->to
            << ", count=" << s->count << "}" << std::endl;
}
S* inc(S *s) {
  s->count += 1;
  return s;
}
} // extern "C"