#ifndef HOME_ROLLED_COROUTINES_H_
#define HOME_ROLLED_COROUTINES_H_

#include <stdbool.h>
#include <stdint.h>

struct context {
  uint64_t rsp;
  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t rbx;
  uint64_t rbp;
};

#ifdef __cplusplus
extern "C" {
#endif

bool swap(struct context *from, struct context *to);
void stop(struct context *from, struct context *to);
bool start(struct context *from, struct context *to, void *arg_ptr,
           void *return_ptr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HOME_ROLLED_COROUTINES_H_
