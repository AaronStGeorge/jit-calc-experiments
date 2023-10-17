#include <bits/types/stack_t.h>
#include <signal.h>
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

// Holds ucontext's associated with user_fn and user_fn_runner
struct user_fn_state {
  ucontext_t uctx_main;
  ucontext_t uctx_user_fn;
  ucontext_t uctx_cleanup;
  stack_t stack;
};

// This is our pretend user_fn
static void fib(struct user_fn_state *user_fn_state) {
  printf("%s: started\n", __func__);
  while (1) {
  }
  printf("%s: returning\n", __func__);
}

// This will eventually be used to decide what return value to use
static void cleanup(struct user_fn_state *user_fn_state) {
  printf("%s: started\n", __func__);
  printf("%s: returning\n", __func__);
}

// Using swapcontext like this in a signal handler is definitely not kosher, see
// man signal-safety(7). That being said, if I understand the problem with using
// swapcontext in a signal handler like this, the danger with doing this is
// basically that the signal might interrupt a non-reentrant function then swap
// into code that calls that function. As fib just does math and doesn't call
// any functions that make system calls, we're *likely* safe.
static void timer_signal_handler(int signum, siginfo_t *info, void *context) {
  struct user_fn_state *user_fn_state = info->si_value.sival_ptr;
  swapcontext(&user_fn_state->uctx_user_fn, &user_fn_state->uctx_cleanup);
}

int user_fn_runner(struct user_fn_state *user_fn_state, void (*user_fn)()) {
  { // setup contexts
    // 4 4k pages for cleanup function, it'll be fine
    uint8_t cleanup_stack[1024 * 4 * 4];

    // setup uctx for user_fn
    if (getcontext(&user_fn_state->uctx_user_fn) == -1) {
      handle_error("getcontext");
    }
    user_fn_state->uctx_user_fn.uc_link = &user_fn_state->uctx_cleanup;
    user_fn_state->uctx_user_fn.uc_stack = user_fn_state->stack;
    makecontext(&user_fn_state->uctx_user_fn, user_fn, 1, &user_fn_state);

    // setup uctx for cleanup function
    if (getcontext(&user_fn_state->uctx_cleanup) == -1) {
      handle_error("getcontext");
    }
    user_fn_state->uctx_cleanup.uc_stack.ss_sp = cleanup_stack;
    user_fn_state->uctx_cleanup.uc_stack.ss_size = sizeof(cleanup_stack);
    user_fn_state->uctx_cleanup.uc_link = &user_fn_state->uctx_main;
    makecontext(&user_fn_state->uctx_cleanup, (void(*))cleanup, 1,
                &user_fn_state);
  }

  timer_t timerid;
  { // setup timer

    // setup signal action handler. This is responsible for actually dispatching
    // to interrupt handler.
    struct sigaction sa;
    // sa_mask causes signals received  while handler is executing to be
    // deferred until handler returns. Our handler is attached to the SIGPROF
    // signal, so masking it will prevent our handler interrupting itself. We
    // likely don't need to worry about that since our handler is only run once,
    // but doesn't hurt.
    sigemptyset(&sa.sa_mask);         // get empty mask
    sigaddset(&sa.sa_mask, SIGRTMIN); //
    // SA_RESTART - restart system calls like read and write if interrupted by
    //              handler.
    // SA_SIGINFO - use 3 argument version of handler (that includes )
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
    // How 300 microseconds was chosen:
    //  - 30 fps equates to 33.33 milliseconds per frame.
    //  - With some overhead that means we get about 30 milliseconds per frame.
    //  - To run 100 robots in 30 milliseconds, each gets 300 microseconds.
    its.it_value.tv_nsec = 300 * 1000; // nanoseconds = microseconds * 1000;
    its.it_value.tv_nsec *= 1000;      // slow it down
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;

    if (timer_settime(timerid, 0, &its, NULL) == -1) {
      handle_error("timer_settime");
    }
  }

  printf("%s: swapcontext(&thing.uctx_main, &thing.uctx_user_fn)\n", __func__);
  if (swapcontext(&user_fn_state->uctx_main, &user_fn_state->uctx_user_fn) ==
      -1) {
    handle_error("swapcontext");
  }

  if (timer_delete(timerid) == -1) {
    perror("timer_delete");
  }
  printf("%s: exiting\n", __func__);
  return 0;
}

int main(int argc, char *argv[]) {
  // 1 Gb stack to support a *lot* of recursion
  size_t user_fn_stack_num_bytes = 1024 * 1024 * 1024;
  void *user_fn_stack = calloc(user_fn_stack_num_bytes, sizeof(uint8_t));

  struct user_fn_state user_fn_state = {
      .stack = {.ss_sp = user_fn_stack, .ss_size = user_fn_stack_num_bytes}};

  for (int i = 0; i < 10; i++) {
    user_fn_runner(&user_fn_state, fib);
  }

  free(user_fn_stack);
  return 0;
}