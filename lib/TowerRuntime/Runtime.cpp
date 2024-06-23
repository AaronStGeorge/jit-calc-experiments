#include "mlir/ExecutionEngine/CRunnerUtils.h"
#include "mlir/ExecutionEngine/RunnerUtils.h"
#include <cstdint>
#include <iostream>

typedef struct {
  uint64_t from;
  uint64_t to;
  uint64_t count;
} S;

extern "C" {
void ding(S *s) {
  std::cout << "ding: {from=" << s->from << ", to=" << s->to
            << ", count=" << s->count << "}" << std::endl;
}
void inc(S *s) {
  s->count += 1;
}
} // extern "C"