#include "types.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "uci.h"





static inline int mvv_lva(struct GameState* Game, Move move) {


    static const int piece_ranks[7] = {1, 1, 2, 3, 4, 5, 6};

    int eval;
    int start_rank = get_from_square(move) / 8;
    int start_file = get_from_square(move) % 8;
    int target_rank = get_to_square(move) / 8;
    int target_file = get_to_square(move) % 8;

    int attacking_piece = Game->board[start_rank][start_file];
    int defending_piece = Game->board[target_rank][target_file];

    int attacking_value = piece_ranks[abs(attacking_piece)];
    int defending_value = piece_ranks[abs(defending_piece)];

    // Assert it's correct values (spots a suprising amount of buGame)
    assert(attacking_value > 0 && attacking_value <= 6);
    assert(defending_value > 0 && defending_value <= 6);

    if (0 && defending_value < attacking_value) {
        int enemy_pawn = Game->side_to_move ? -PAWN : PAWN;
        int pawn_dir = Game->side_to_move ? 1 : -1;

        
        if ((target_rank + pawn_dir >= 0 && target_rank + pawn_dir < 8)) {
            if (((target_file + 1 < 8) && Game->board[target_rank + pawn_dir][target_file + 1] == enemy_pawn) 
                || (target_file - 1 >= 0 && Game->board[target_rank + pawn_dir][target_file - 1] == enemy_pawn)) return -10000000 + 10 * defending_value - attacking_value;
        }
        
    }

    // Standard mvv_lva
    eval = 10 * (defending_value) - attacking_value;

    return eval;
}

void evaluate_moves_quiescence(struct GameState* Game, Move legal_moves[256], int total_legal_moves, int evals[256], int history_table[64][64]) {


    for (int i = 0; i < total_legal_moves; i++) {

       // If capture -> sort with mvv_lva
        if (get_capture_flag(legal_moves[i]) == CAPTURE) {
            evals[i] =  10000000 + mvv_lva(Game, legal_moves[i]);
        }
        else { // else history table and killer moves
            int ss = get_from_square(legal_moves[i]);
            int ts = get_to_square(legal_moves[i]);
            evals[i] += history_table[ss][ts];
        }
    }
}



void sort_moves_quiescence(struct GameState* Game, Move legal_moves[256], int total_legal_moves, int eval_order[256], int history_table[64][64]) {

    int evals[256] = {0};

    for (int i = 0; i < total_legal_moves; i++) {
        eval_order[i] = i;

       // If capture -> sort with mvv_lva
        if (get_capture_flag(legal_moves[i]) == CAPTURE) {
            evals[i] =  10000000 + mvv_lva(Game, legal_moves[i]);
        }
        else { // else history table and killer moves
            int ss = get_from_square(legal_moves[i]);
            int ts = get_to_square(legal_moves[i]);
            evals[i] += history_table[ss][ts];
        }
    }
    // Sort
    for (int i = 1; i < total_legal_moves; i++) {
        int idx_copy = eval_order[i];
        int eval_copy = evals[i];
        int j = i - 1;
        while (j >= 0 && eval_copy > evals[j]) {
            eval_order[j+1] = eval_order[j];
            evals[j+1] = evals[j];
            j--;
        }
        eval_order[j+1] = idx_copy;
        evals[j+1] = eval_copy;
    }
}


void evaluate_moves(struct GameState* Game, Move legal_moves[256], int total_legal_moves, struct NodeState Node, int eval[256], struct SearchContext* Search, Move hash_move) {


    for (int i = 0; i < total_legal_moves; i++) {

        if (moves_match(legal_moves[i], hash_move)) {
            eval[i] += 100000000;
            continue;
        }


        // If capture -> sort with mvv_lva
        if (get_capture_flag(legal_moves[i]) == CAPTURE) {
            eval[i] +=  10000000 + mvv_lva(Game, legal_moves[i]);
        }
        else { // else history table and killer moves
            int ss = get_from_square(legal_moves[i]);
            int ts = get_to_square(legal_moves[i]);
            eval[i] += Search->history_table[ss][ts];

            for (int k = 0; k < 2; k++) {
                if (moves_match(legal_moves[i], Search->killer_moves[Node.ply][k])) {
                        eval[i] += (2 - k) * 1000000;
                        break;
                }
                else if (Node.ply >= 2) {
//                    if (moves_match(legal_moves[i], Search->killer_moves[Node.ply - 2][k])) eval[i] += 10000;
                }
            }
        }
    }
}
