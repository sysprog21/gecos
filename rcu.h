#ifndef __RCU_H
#define __RCU_H

#include "lock.h"
#include "test.h"

void quiescent_state(int);

void free_node_later(int, node_t *);

#endif
