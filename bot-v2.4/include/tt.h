#ifndef TT_H
#define TT_H

#include <stdint.h>
enum TT_FLAG { EXACT = 1, UPPERBOUND = 2, LOWERBOUND = 3 };

struct tt_entry {
    uint64_t zobrist_key;
    int best_move;
    int16_t eval;
    uint8_t depth;
    uint8_t flag;
};


extern struct tt_entry *TT;
extern int TT_SIZE;

#endif
