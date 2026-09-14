#ifndef SORT_MOVES_H
#define SORT_MOVES_H
void sort_moves_quiescence(struct GameState* gs, Move legal_moves[256], int total_legal_moves, Move last_played_move, int eval_order[256], int history_table[64][64]);
void evaluate_moves(struct GameState* gs, Move legal_moves[256], int total_legal_moves, struct NodeState Node, int eval[256], struct SearchContext* Search, Move hash_move);


#endif

