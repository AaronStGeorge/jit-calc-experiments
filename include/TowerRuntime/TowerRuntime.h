#ifndef TOWER_RUNTIME_H
#define TOWER_RUNTIME_H

#include <cstdint>

typedef struct {
  uint64_t from;
  uint64_t to;
  uint64_t count;
} S;


#ifdef __cplusplus
extern "C" {
#endif

void dump(S *s);
S* inc(S *s);

#ifdef __cplusplus
} // extern "C"
#endif


#endif // TOWER_RUNTIME_H
