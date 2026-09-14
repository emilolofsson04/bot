#include <stdlib.h>
#include "structs.h"
#include <stdio.h>
#include <time.h>

#include "movegen.h"
#include "playmove.h"
#include "unplaymove.h"
#include "eval.h"
#include "evalboards.h"
#include "sort_moves.h"
#include "uci.h"

#include <assert.h>





static int quiescence_search(struct GameState* gs, int ply, uint32_t* last_played_move, int alpha, int beta, struct EvalBoards* eb, struct EngineStats* es, int checkBuffer) {


    (es->qnodes)++;
    if (gs->halfmove_clock >= 100) return 0;
    int limit = (gs->halfmove_clock < gs->ply) ? gs->halfmove_clock : gs->ply;
    for (int back_ply = 2; back_ply <= limit; back_ply += 2) {
        if (gs->zobrist_hash_3fold == gs->zobrist_hash_3fold_history[gs->ply - back_ply]) return 0;
    }


    // Initialize
    int total_legal_moves = 0;
    int branchEval = 0;
    int eval_order[256];
    uint32_t legal_moves[256];

    // Evaluate if the king is safe
    int inCheck = 0;
    int king_square = (gs->side_to_move == SIDE_WHITE) ? gs->white_king_square : gs->black_king_square;
    int king_rank = king_square / 8;
    int king_file = king_square % 8;
    if (!is_king_safe(gs->Board, king_rank, king_file, gs->side_to_move)) {
        inCheck = 1;
    }

    // Only allow so many check exstensions  
    if (inCheck) {
        if (checkBuffer <= 0) {
            is_king_safe(gs->Board, king_rank, king_file, gs->side_to_move); 
            return Eval(gs, eb);
        }
        checkBuffer--; // Consume one check from the buffer
    }


    // Only allow StandPat if King is safe, else capture/move needs to be played
    int StandPat = -100000;
    if (!inCheck) {
    // evaluate position and set StandPat
        StandPat = Eval(gs, eb);
        // If standpat >= beta, black wont allow this position to be reached -> break
        if (StandPat >= beta) {
            is_king_safe(gs->Board, king_rank, king_file, gs->side_to_move);
            (es->standpat_cutoff)++;
            return beta;
        }
        // If StandPat > alpha, we can garantee stand pat if not capture in overtime is made
        if (StandPat  > alpha) {
            alpha = StandPat;
        }
    }

    // Store best value
    int best = StandPat;


    // If not in check, find only captures
    if (!inCheck) {
        total_legal_moves = generate_legal_captures(gs, legal_moves);
        sort_moves(gs, legal_moves, total_legal_moves, last_played_move, eval_order); 
    }

    // In check find all legal evasions 
    if (inCheck) {
        total_legal_moves = generate_legal_moves(gs, legal_moves);
        sort_moves(gs, legal_moves, total_legal_moves, last_played_move, eval_order); 
    }

    for (int k = 0; k < total_legal_moves; k++) {
        struct UndoInfo ui;

        // eval_order given by sort_moves()
        Move move = legal_moves[eval_order[k]];

        play_move(move, gs, &ui); 

        // Swap colour and continue branching
        branchEval = -quiescence_search(gs, ply + 1, &move, -beta, -alpha, eb, es, checkBuffer);

        // Evaluate alpha beta values
        if (branchEval > best) {
            best = branchEval;
        }
        if (branchEval > alpha) {
            alpha = branchEval;
        }
        // Break fi alpha beta condition is reached
        if (beta <= alpha) {
            if (get_capture_flag(move) == 1) {
                (es->capture_cutoff)++;
            }
            else {
                (es->quiet_cutoff)++;
            }
            unplay_move(move, gs, &ui);
            break;
        }
        unplay_move(move, gs, &ui);

    }



    // If not legal moves - Return basecase or checkmate
    if (total_legal_moves == 0) {
        // If in check we have found all legal moves due to check evasion. So we can garantee its mate
        if (inCheck) {
            (es->qcheckmate)++;
            return -100000 +  (ply);
        }
        else { // We dont know if its stalemate or if simply no captures exist
            return StandPat;
        }
    }

    // Return the best moves evaluation
    assert(best >= StandPat);
    return best;
} 



static int branch(struct GameState* gs, uint32_t* last_played_move, int ply, int depth, int alpha, int beta, struct EvalBoards* eb, struct EngineStats* es, uint32_t pvTable[64][64], int pvLength[64], uint32_t old_pv_line[64], int old_pv_length, int on_pv, uint32_t killer_moves[64][2], int history_table[64][64]) {

    // Continue with quiescence
    if (depth == 0) {
        return quiescence_search(gs, ply, last_played_move, alpha, beta, eb, es, 3);
    }
     (es->nodes)++;

    if (gs->halfmove_clock >= 100) return 0;
    int limit = (gs->halfmove_clock < gs->ply) ? gs->halfmove_clock : gs->ply;
    for (int back_ply = 2; back_ply <= limit; back_ply += 2) {
        if (gs->zobrist_hash_3fold == gs->zobrist_hash_3fold_history[gs->ply - back_ply]) return 0;
    }


    // Initialize
    int eval_order[256];
    int total_legal_moves = 0;
    uint32_t legal_moves[256];
    
    int best = -100000;
    int branch_eval = best;



    // Fine all legal moves
    total_legal_moves = generate_legal_moves(gs, legal_moves);

    // Sort the moves, returning eval_order[] 
    sort_moves_v2(gs, legal_moves, total_legal_moves, last_played_move, eval_order, old_pv_line, old_pv_length, on_pv, ply, killer_moves, history_table); 
    for (int k = 0; k < total_legal_moves; k++) {

        pvLength[ply + 1] = ply + 1;

        Move move = legal_moves[eval_order[k]];

        if (on_pv && k == 0 && ply < old_pv_length) {
            on_pv = 1;
        }
        else {
            on_pv = 0;
        }

        struct UndoInfo ui;
        play_move(move, gs, &ui); 
                                                              
        // Swap colour and continue branching
        branch_eval = -branch(gs, &move, ply + 1, depth - 1, -beta, -alpha, eb, es, pvTable, pvLength, old_pv_line, old_pv_length, on_pv, killer_moves, history_table);

        // Evaluate alpha beta values
        if (branch_eval > best) {
            best = branch_eval; 
        }
        if (best > alpha) {
            alpha = best;

            // Update pv
            pvTable[ply][ply] = move;

            // Copy child line
            for (int i = ply + 1; i < pvLength[ply + 1]; i++) {
                pvTable[ply][i] = pvTable[ply + 1][i];
            }
            pvLength[ply] = pvLength[ply + 1];
        }

        unplay_move(move, gs, &ui);

        // Break fi alpha beta condition is reached
        if (beta <= alpha) {
            if (get_capture_flag(move) == 0) {
                killer_moves[ply][1] = killer_moves[ply][0];
                killer_moves[ply][0] = move;

                int ss = get_from_square(move);
                int ts = get_to_square(move);
                history_table[ss][ts] += (depth * depth + 5)/5;
                if (history_table[ss][ts] > 999999) {
                    history_table[ss][ts] = 999999;
                }
                (es->quiet_cutoff)++;
            }
            else {
                (es->capture_cutoff)++;
            } 
            break;
        }

    }

    // If not legal moves find, check if chekmate or default to stalemate   
    if (total_legal_moves == 0) {

        int king_square = (gs->side_to_move == SIDE_WHITE) ? gs->white_king_square : gs->black_king_square;
        int king_rank = king_square / 8;
        int king_file = king_square % 8;

        // No legal moves found, reduce pv length
        pvLength[ply] = ply; 
        if (!is_king_safe(gs->Board, king_rank, king_file, gs->side_to_move)) {
            (es->checkmate)++;
            return -100000 +  (ply);
        }

        (es->stalemate)++;
        return 0;
    }

    // Find and return the best moves evaluation
    return best;
}


uint32_t iterative_deepening(struct GameState gs, int max_depth, int max_time) {

     /*
     Finds the optimal move at a certain depth,
    using negamax and iterative deepening.
    */

    if (max_time != -1) max_depth = 20;
    // branch evaluation
    int branch_eval;
    int who2play = (gs.side_to_move == SIDE_WHITE) ? 1 : -1;
    

    // Initialize evaluation boards
    struct EvalBoards EB = defEvalBoards();

    // Find all legal moves 
    uint32_t legal_moves[256];
    int total = generate_legal_moves(&gs, legal_moves);
    
    // Stats for printing
    struct EngineStats es;

    // If non exist, evaluate of we are in mate or stalemate
    if (total == 0) {
        uint32_t nullMove = {0};
        return nullMove;
    }

    // Turn into RootMoves conatining move and eval for sorting
    struct RootMove root_legal_moves[total];
    for (int i = 0; i < total; i++) {
        root_legal_moves[i].Move = legal_moves[i];
        root_legal_moves[i].eval = -100000;
    }
    
    // Triangular pv table
    uint32_t pvTable[64][64];
    int pvLength[64];

    uint32_t old_pv_line[64];
    int old_pv_length = 0;         
    int on_pv = 1;

    uint32_t killer_moves[64][2] = {0};

    // Start timer and reset node count
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    uint64_t total_nodes = 0;
    int history_table[64][64] = {0};

    for (int iterationdepth = 1; iterationdepth <= max_depth; iterationdepth++) { 

        // Used for estimating time remaining
        int previous_nodes = total_nodes;
        double bf = 1;
        es.nodes = 0;
        es.qnodes = 0;

        pvLength[1] = 1;

        // Initialize alpha beta
        int alpha = -100000;
        int beta =   100000;
 
        // Iterate over legalmoves
        for (int k = 0; k < total; k++) {
            // Play move 
            struct UndoInfo ui;
            play_move(root_legal_moves[k].Move, &gs, &ui);

            // Spaw color and call branch function
            if (k == 0) {
                on_pv = 1;
            }
            else {
                on_pv = 0;
            }

            
            branch_eval = -branch(&gs, &root_legal_moves[k].Move, 1, iterationdepth - 1, -beta, -alpha, &EB, &es, pvTable, pvLength, old_pv_line, old_pv_length, on_pv, killer_moves, history_table);

            // Set RootMoves evaluation
            root_legal_moves[k].eval = branch_eval;

            // Unplay the move
            unplay_move(root_legal_moves[k].Move, &gs, &ui);

            // Evaluate alpha beta
            if (branch_eval > alpha) {
                alpha = branch_eval;

                // Update pv
                pvTable[0][0] = root_legal_moves[k].Move;

                // Copy child line
                for (int i = 1; i < pvLength[1]; i++) {
                    pvTable[0][i] = pvTable[1][i];
                }
                pvLength[0] = pvLength[1];
            }

            // break if alpha beta condition reached
            if (beta <= alpha) {
                break;
            }

        }
        // Sort RootMoves for next iteration
        struct RootMove movecopy = {0};
        for (int moves = 1; moves < total; moves++) {
            movecopy = root_legal_moves[moves];
            int j = moves - 1;
            while (j >= 0 && movecopy.eval > root_legal_moves[j].eval) {
                root_legal_moves[j+1] = root_legal_moves[j]; 
                j--;
            }
            root_legal_moves[j + 1] = movecopy;
        }

        // Refactor old pv line
        old_pv_length = pvLength[0];
        for (int i = 0; i < pvLength[0]; i++) {
            old_pv_line[i] = pvTable[0][i];
        }

        // End timer
        clock_gettime(CLOCK_MONOTONIC, &now);
        double time_taken =
            (now.tv_sec - start.tv_sec) +
            (now.tv_nsec - start.tv_nsec) / 1e9;

        total_nodes += es.qnodes + es.nodes;
        write_info(root_legal_moves[0].eval * who2play, iterationdepth, total_nodes, time_taken, pvLength[0], pvTable);

        if (max_time != -1) {
            if (previous_nodes) {
                bf = (double)total_nodes/previous_nodes;
            }

            if (1.2 * time_taken * bf * 1000 > max_time) {
                break;
            }
        }

    }

    // Find and return argument of the best move in position
    return root_legal_moves[0].Move;
}

