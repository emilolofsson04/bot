#ifndef SORT_MOVES_H
#define SORT_MOVES_H
void sort_moves(struct GameState* gs, uint32_t legal_moves[256], int total_legal_moves, uint32_t* last_played_move, int eval_order[256], int history_table[64][64]);
void evaluate_moves(struct GameState* gs, uint32_t legal_moves[256], int total_legal_moves, uint32_t* last_played_move, int eval[256], uint32_t old_pv_line[64], int old_pv_length, int on_pv, int ply, uint32_t killer_moves[64][2], int history_table[64][64], Move hash_move);


#endif

