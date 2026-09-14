#include "structs.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "uci.h"





static inline int mvv_lva(struct GameState* gs, Move move, Move last_played_move) {

    static const int pieceValues[7] = {1, 1, 3, 3, 5, 9, 10};


    int eval;
    int start_rank = get_from_square(move) / 8;
    int start_file = get_from_square(move) % 8;
    int target_rank = get_to_square(move) / 8;
    int target_file = get_to_square(move) % 8;

    int attacking_piece = gs->board[start_rank][start_file];
    int defending_piece = gs->board[target_rank][target_file];

    int attacking_value = pieceValues[abs(attacking_piece)];
    int defending_value = pieceValues[abs(defending_piece)];

    // Assert it's correct values (spots a suprising amount of bugs)
    assert(attacking_value > 0 && attacking_value < 11);
    assert(defending_value > 0 && defending_value < 11);

    // Standard mvv_lva
    eval = 10 * (defending_value) - attacking_value;

    // If move is a recapture, promote it to ceo
    if (last_played_move && (target_rank == (get_to_square(last_played_move) / 8)) && (target_file == (get_to_square(last_played_move) % 8))) {
        eval += 1;
    }
    return eval;
}


void sort_moves_quiescence(struct GameState* gs, Move legal_moves[256], int total_legal_moves, Move last_played_move, int eval_order[256], int history_table[64][64]) {
    
    int evals[256] = {0};

    for (int i = 0; i < total_legal_moves; i++) {
        eval_order[i] = i;

       // If capture -> sort with mvv_lva
        if (get_capture_flag(legal_moves[i]) == CAPTURE) {
            evals[i] =  100000000 + mvv_lva(gs, legal_moves[i], last_played_move);
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

void evaluate_moves(struct GameState* gs, Move legal_moves[256], int total_legal_moves, struct NodeState Node, int eval[256], struct SearchContext* Search, Move hash_move) {

    int pv_moves = 0;

    for (int i = 0; i < total_legal_moves; i++) {

        // If the pv line, go first
        if (0 && Node.on_pv && Search->old_pv_length > Node.ply) {
            if (moves_match(legal_moves[i], Search->old_pv_line[Node.ply])) {
                eval[i] += 1000000000;
                pv_moves++;
                assert(pv_moves <= 1);
                continue;
            }
        }

        if (moves_match(legal_moves[i], hash_move)) {
            eval[i] += 100000000;
            continue;
        }


        // If capture -> sort with mvv_lva
        if (get_capture_flag(legal_moves[i]) == CAPTURE) {
            eval[i] +=  10000000 + mvv_lva(gs, legal_moves[i], Node.last_played_move);
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
