#ifndef ZOBRIST_H
#define ZOBRIST_H
#include <stdint.h>
extern uint64_t zobrist_pieces[12][64];
extern uint64_t zobrist_castling_rights[16];
extern uint64_t zobrist_side_to_move;
extern uint64_t zobrist_en_passant_file[8];
void init_zobrist();
#endif


