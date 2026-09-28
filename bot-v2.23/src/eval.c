#include "types.h"
uint64_t passed_pawn_masks[2][64];

void init_evaluation_masks(void) {
    const uint64_t FILE_A = 0x0101010101010101ULL;

    for (int sq = 0; sq < 64; sq++) {

        int r = sq / 8;
        int f = sq % 8;

        uint64_t files = FILE_A << f;
        if (f > 0) files |= (FILE_A << (f - 1));
        if (f < 7) files |= (FILE_A << (f + 1));

        uint64_t w_above = (r >= 7) ? 0ULL : (~0ULL << (8 * (r + 1)));
        passed_pawn_masks[SIDE_WHITE][sq] = files & w_above;

        uint64_t b_below = (r <= 0) ? 0ULL : ((1ULL << (8 * r)) - 1ULL);
        passed_pawn_masks[SIDE_BLACK][sq] = files & b_below;
    }
}


