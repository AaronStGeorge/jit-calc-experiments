#include <assert.h>
#include <bits/types/stack_t.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

// This represents what would be a game function
static int fast_fib(int n) {
  int i = 0;
  int j = 1;

  for (int c = 0; c < n; c++) {
    int tmp = i + j;
    i = j;
    j = tmp;
  }
  return i;
}

// This represents what would be a user function
static int slow_fib(int n) {
  if (n <= 1) {
    return n;
  }
  return slow_fib(n - 2) + slow_fib(n - 1);
}

typedef struct {
  uint32_t high;
  uint32_t low;
} unpacked_uint64_t;

uint64_t pack(unpacked_uint64_t unpacked) {
  return (uint64_t)unpacked.high << 32 | unpacked.low;
}

unpacked_uint64_t unpack(uint64_t packed) {
  unpacked_uint64_t unpacked = {.low = (uint32_t)packed,
                                .high = (uint32_t)(packed >> 32)};
  return unpacked;
}

// Blob of state associated with user_fn, could probably be broken up a bit.
struct user_fn_state {
  int default_or_previous_return_value;
  int return_value;
  bool needs_new_context;
  ucontext_t uctx_main;
  ucontext_t uctx_user_fn;
  stack_t stack;
};

// We would probably want to auto-generate wrapper functions like this
static void fib_wrapper(int did_return_high, int did_return_low,
                        int return_value_high, int return_value_low,
                        int arg_high, int arg1_low) {
  int n;
  {
    unpacked_uint64_t arg_packed = {.high = arg_high, .low = arg1_low};
    int *arg = (int *)pack(arg_packed);
    n = *arg;
  }

  int *return_value;
  {
    unpacked_uint64_t return_value_packed = {.high = return_value_high,
                                             .low = return_value_low};
    return_value = (int *)pack(return_value_packed);
  }

  bool *did_return;
  {
    unpacked_uint64_t did_return_packed = {.high = did_return_high,
                                           .low = did_return_low};
    did_return = (bool *)pack(did_return_packed);
  }

  *return_value = slow_fib(n);
  // Writing a value of machine word size to a ptr and reading it in same thread
  // should be atomic. did_return functions sort of like a lock around
  // return_value which could (but doesn't in this case) require multiple write
  // instructions (that might be interrupted by signal handler).
  *did_return = true;
}

// Using swapcontext like this in a signal handler is definitely not kosher, see
// man signal-safety(7). That being said, if I understand the problem with using
// swapcontext in a signal handler like this, the danger with doing this is
// basically that the signal might interrupt a non-reentrant function then swap
// into code that calls that function. As fib just does math and doesn't call
// any functions that make system calls, we're *likely* safe.
static void timer_signal_handler(int signum, siginfo_t *info, void *context) {
  struct user_fn_state *user_fn_state = info->si_value.sival_ptr;
  swapcontext(&user_fn_state->uctx_user_fn, &user_fn_state->uctx_main);
}

// run_user_fn runs user_fn from starting state in user_fn_state for
// runtime_nsec nanoseconds. run_user_fn returns result if user_fn completes,
// and previous result (from user_fn_state) if user_fn doesn't complete.
int run_user_fn(long int runtime_nsec, struct user_fn_state *user_fn_state,
                void (*user_fn)(), int user_fn_arg) {
  // setup uctx_user
  if (user_fn_state->needs_new_context) {
    user_fn_state->needs_new_context = false;
    // setup uctx for user_fn
    if (getcontext(&user_fn_state->uctx_user_fn) == -1) {
      handle_error("getcontext");
    }

    // makecontext only supports 32 bit arguments
    // https://github.com/bminor/glibc/blob/5ef608f36493c5d711418c5d31a7ebe710decc6e/sysdeps/unix/sysv/linux/x86_64/makecontext.c#L31-L52
    // so we need to break 64 bit pointers over two ints then stich them back
    // together. Obnoxious, but whatever.
    unpacked_uint64_t did_return_unpacked =
        unpack((uint64_t)&user_fn_state->needs_new_context);
    unpacked_uint64_t return_value_unpacked =
        unpack((uint64_t)&user_fn_state->return_value);
    // Doesn't need to be ptr, pass as ptr just for uniformity.
    unpacked_uint64_t arg_unpacked = unpack((uint64_t)&user_fn_arg);
    user_fn_state->uctx_user_fn.uc_link = &user_fn_state->uctx_main;
    user_fn_state->uctx_user_fn.uc_stack = user_fn_state->stack;
    makecontext(
        &user_fn_state->uctx_user_fn, user_fn, 6,
        /*did_return*/ did_return_unpacked.high, did_return_unpacked.low,
        /*return_value*/ return_value_unpacked.high, return_value_unpacked.low,
        /*arg*/ arg_unpacked.high, arg_unpacked.low);
  }

  timer_t timerid;
  { // setup timer

    // setup signal action handler. This is responsible for actually dispatching
    // to interrupt handler.
    struct sigaction sa;
    // sa_mask causes signals received  while handler is executing to be
    // deferred until handler returns. Our handler is attached to the SIGRTMIN
    // signal, so masking it will prevent our handler interrupting itself. We
    // likely don't need to worry about that since our handler is only run once,
    // but doesn't hurt.
    sigemptyset(&sa.sa_mask);         // get empty mask
    sigaddset(&sa.sa_mask, SIGRTMIN); //
    // SA_RESTART - restart system calls like read and write if interrupted by
    //              handler.
    // SA_SIGINFO - use 3 argument version of handler function, there's also a 1
    //              argument version
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sa.sa_sigaction = timer_signal_handler;
    if ((sigaction(SIGRTMIN, &sa, NULL)) != 0) {
      handle_error("oh no, installing a signal handler can fail?");
    }

    // Setup timer. This timer determines when interrupt handler that swaps away
    // from user_fun should fires.
    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = user_fn_state;
    if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
      handle_error("timer_create");
    }

    struct itimerspec its;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = runtime_nsec;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;

    if (timer_settime(timerid, 0, &its, NULL) == -1) {
      handle_error("timer_settime");
    }
  }

  // "call" user_fn.
  if (swapcontext(&user_fn_state->uctx_main, &user_fn_state->uctx_user_fn) ==
      -1) {
    handle_error("swapcontext");
  }
  // Either through timer expiry, or function completion we've reached the end
  // of this call. The timer is no longer required.
  if (timer_delete(timerid) == -1) {
    perror("timer_delete");
  }

  // We only need a new context if user_fn completed
  if (user_fn_state->needs_new_context) {
    user_fn_state->default_or_previous_return_value =
        user_fn_state->return_value;
  }

  return user_fn_state->return_value;
}

int main(int argc, char *argv[]) {
  // test of pack and unpack function
  uint64_t v = 0xFEDCBA9876543210;
  assert(v == pack(unpack(v)));

  // 1 Gb stack to support a *lot* of recursion
  size_t user_fn_stack_num_bytes = 1024 * 1024 * 1024;
  void *user_fn_stack = calloc(user_fn_stack_num_bytes, sizeof(uint8_t));

  struct user_fn_state user_fn_state = {
      .needs_new_context = true,
      .stack = {.ss_sp = user_fn_stack, .ss_size = user_fn_stack_num_bytes}};

  // How 300 microseconds was chosen:
  //  - 30 fps equates to 33.33 milliseconds per frame.
  //  - With some overhead that means we get about 30 milliseconds to compute
  //    a frame.
  //  - To run 100 robots in 30 milliseconds, each gets 300 microseconds.
  long int runtime_nsec = 300 * 1000; // nanoseconds = microseconds * 1000;

  printf("Fibonacci game!\n");
  printf("%6s %6s\n", "game", "user");
  for (int i = 0; i < 30; i++) {
    int slow_roll = run_user_fn(runtime_nsec, &user_fn_state, fib_wrapper, i);
    int fast_roll = fast_fib(i);
    printf("%6d %6d\n", fast_roll, slow_roll);
  }

  free(user_fn_stack);
  return 0;
}