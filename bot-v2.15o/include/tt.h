#ifndef TT_H
#define TT_H
#include <stdint.h>
#include "types.h"

enum TT_FLAG { EXACT = 1, UPPERBOUND = 2, LOWERBOUND = 3 };

struct tt_entry {
    uint64_t zobrist_key;
    Move best_move;
    int16_t eval;
    uint8_t depth;
    uint8_t flag;
    int generation;
};


extern struct tt_entry *TT;
extern int tt_generation;

void init_tt(void);
void reset_tt(void);
void free_tt(void);

#endif
