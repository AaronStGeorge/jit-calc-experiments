#ifndef HOME_ROLLED_COROUTINES_H_
#define HOME_ROLLED_COROUTINES_H_

#include <sys/types.h>

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
