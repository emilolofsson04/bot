#include <stdlib.h>
#include "structs.h"
#include <stdio.h>
#include <time.h>

#include "search.h"
#include "movegen.h"
#include "playmove.h"
#include "unplaymove.h"
#include "eval.h"
#include "evalboards.h"
#include "sortmoves.h"
#include "uci.h"
#include "tt.h"
#include "zobrist.h"
#include <string.h>

#include <assert.h>


int total_generate_moves = 0;
int try_store = 0;
int stored = 0;

static inline int is_game_a_draw(struct GameState* Game) {

    /* returns true if the game is a draw due to repetition of halfmove clock */

    if (Game->halfmove_clock >= 100) return 1;

    int limit = (Game->halfmove_clock < Game->ply) ? Game->halfmove_clock : Game->ply;

    for (int old_ply = 2; old_ply <= limit; old_ply += 2) {
        if (Game->zobrist_hash == Game->zobrist_hash_3fold_history[Game->ply - old_ply]) return 1;
    }
    // Not a draw
    return 0;
}

static inline void pick_best_move_first(int eval[256], Move legal_moves[256], int total_legal_moves, int k) {

    /* Swaps the best evaluated move with the first */

    // Find the best eval out of the move list
    int max_eval = -1;
    int max_arg = k;
    for (int i = k; i < total_legal_moves; i++) {
        if (eval[i] > max_eval) {
            max_eval = eval[i];
            max_arg = i;
        }
    }

    // Swap the best move with the first
    Move temp = legal_moves[k];
    int temp_eval = eval[k];
    eval[k] = eval[max_arg];
    eval[max_arg] = temp_eval;
    legal_moves[k] = legal_moves[max_arg];
    legal_moves[max_arg] = temp;
}


static int quiescence_search(struct GameState* Game, struct NodeState Node, struct SearchContext* Search, int alpha, int beta, int checkBuffer) {


    (Search->es.qnodes)++;
    (Search->nodes)++;
    if (is_game_a_draw(Game)) return 0;

    // Initialize
    int total_legal_moves = 0;
    int branchEval = 0;
    int eval_order[256];
    Move legal_moves[256];

    // Evaluate if the king is safe
    int inCheck = 0;
    int king_square = (Game->side_to_move == SIDE_WHITE) ? Game->white_king_square : Game->black_king_square;
    int king_rank = king_square / 8;
    int king_file = king_square % 8;
    
    if (!is_king_safe(Game->board, king_rank, king_file, Game->side_to_move)) {
        inCheck = 1;
    }
    
    

    // Only allow so many check exstensions  
    if (inCheck) {
        if (checkBuffer <= 0) {
           // return Eval(Game, &Search->eb);
           inCheck = 0;
        }
        checkBuffer--; // Consume one check from the buffer
    }

    // Only allow StandPat if King is safe, else capture/move needs to be played
    int StandPat = NEGATIVE_INFINITY;
    if (!inCheck) {
    // evaluate position and set StandPat
        //StandPat = Eval(Game, &Search->eb);

        StandPat = evaluate_position(Game);

        // If standpat >= beta, black wont allow this position to be reached -> break
        if (StandPat >= beta) {
            (Search->es.standpat_cutoff)++;
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
        total_legal_moves = generate_legal_captures(Game, legal_moves);
        sort_moves_quiescence(Game, legal_moves, total_legal_moves, eval_order, Search->history_table); 
    }
    // In check find all legal evasions 
    if (inCheck) {
        total_legal_moves = generate_legal_moves(Game, legal_moves);
        sort_moves_quiescence(Game, legal_moves, total_legal_moves, eval_order, Search->history_table);
    }
    for (int k = 0; k < total_legal_moves; k++) {

        Move move = legal_moves[eval_order[k]];

        struct UndoInfo ui;
        make_move(move, Game, &ui, &Search->eb); 

        struct NodeState child_node = { .ply = Node.ply + 1, .depth = Node.depth - 1 };
        branchEval = -quiescence_search(Game, child_node, Search, -beta, -alpha, checkBuffer);

        // Evaluate alpha beta values
        if (branchEval > best) {
            best = branchEval;
        }
        if (branchEval > alpha) {
            alpha = branchEval;
        }
        unmake_move(move, Game, &ui);
        if (beta <= alpha) {
            break;
        }

    }



    // If not legal moves - Return basecase or checkmate
    if (total_legal_moves == 0) {
        // If in check we have found all legal moves due to check evasion. So we can garantee its mate
        if (inCheck) {
            (Search->es.qcheckmate)++;
            return -MATE_SCORE +  (Node.ply);
        }
        else { // We dont know if its stalemate or if simply no captures exist
            return StandPat;
        }
    }

    // Return the best moves evaluation
    assert(best >= StandPat);
    return best;
} 

static inline int probe_tt(Move* hash_move, int alpha, int beta,  int* return_eval, struct SearchContext* Search, struct NodeState Node, struct GameState* Game) {

    /* Checks current tt entry and compares zorbist key to find usefull information */

    int tt_index = Game->zobrist_hash & (TT_SIZE - 1);
    struct tt_entry* entry = &TT[tt_index];
    int tt_age = tt_generation - TT[tt_index].generation;

    // If no match, return
    if (!(entry->zobrist_key == Game->zobrist_hash)) return 0;

    if (tt_age == 0) {
        (Search->es.hash_move)++;
        *hash_move = entry->best_move;
    }

    if (tt_age == 0 && entry->depth >= Node.depth) {

        int entry_eval = entry->eval;
        if (entry_eval >= MATE_IN_100_SCORE) entry_eval -= Node.ply;
        if (entry_eval <= -MATE_IN_100_SCORE) entry_eval += Node.ply;

        if (entry->flag == EXACT){
            *return_eval = entry_eval;
            return 1;
        }

        if (entry->flag == UPPERBOUND) {
            if (entry_eval <= alpha) {
                *return_eval = entry_eval;
                return 1;
            }
        }
        if (entry->flag == LOWERBOUND) {
            if (entry_eval >= beta) {
                *return_eval = entry_eval;
                return 1;
            }
        }
    }
    return 0;

}

static inline void store_position_tt(struct NodeState Node, struct GameState* Game, Move best_move, int best, int old_alpha, int old_beta) {

    /* Stores the current position in the tt */


    int tt_index = Game->zobrist_hash & (TT_SIZE - 1);
    struct tt_entry *entry = &TT[tt_index];

    int tt_age = tt_generation - entry->generation;

    try_store++;

    if (tt_age > 0 || Node.depth >= entry->depth) {
        stored++;

        entry->zobrist_key = Game->zobrist_hash;
        entry->best_move = best_move;
        entry->depth = Node.depth;
        entry->generation = tt_generation;

        int entry_eval = best;

        if (entry_eval >= MATE_IN_100_SCORE)
            entry_eval += Node.ply;

        if (entry_eval <= -MATE_IN_100_SCORE)
            entry_eval -= Node.ply;

        entry->eval = entry_eval;

        if (best <= old_alpha) {
            entry->flag = UPPERBOUND;
        }
        else if (best >= old_beta) {
            entry->flag = LOWERBOUND;
        }
        else {
            entry->flag = EXACT;
        }
    }
}

static inline void update_best(int *best, Move* best_move, Move move, int branch_eval) {

    /* Updates the best value if beaten */

    if (branch_eval > *best) {
        *best_move = move;
        *best = branch_eval;
    }
}

static inline void update_alpha_and_pv(struct NodeState* Node, int* alpha, struct SearchContext* Search, int best, Move move) {

    /* Updates alpha if beaten, also adjusting pv line */

    if (best > *alpha) {
        *alpha = best;

        // Update pv
        Search->pvTable[Node->ply][Node->ply] = move;

        // Copy child line
        for (int i = Node->ply + 1; i < Search->pvLength[Node->ply + 1]; i++) {
            Search->pvTable[Node->ply][i] = Search->pvTable[Node->ply + 1][i];
        }
        Search->pvLength[Node->ply] = Search->pvLength[Node->ply + 1];
    }
}

static inline int is_alpha_beta_cutoff(int alpha, int beta) {

    return beta <= alpha;
}

static inline void record_cutoff(struct NodeState Node, struct SearchContext* Search, Move move, Move hash_move) {

    /* Updates heuristics based on cutoff */


    if (get_capture_flag(move) == QUIET) {

        Search->killer_moves[Node.ply][1] = Search->killer_moves[Node.ply][0];
        Search->killer_moves[Node.ply][0] = move;

        int ss = get_from_square(move);
        int ts = get_to_square(move);
        Search->history_table[ss][ts] += (Node.depth * Node.depth + 5) / 5;
        if (Search->history_table[ss][ts] > 999999) {
            Search->history_table[ss][ts] = 999999;
        }
    }
}


static inline int handle_no_legal_moves(struct GameState* Game, struct SearchContext* Search, struct NodeState Node) {

    /* Evaluate if check mate, else defualt Stalemate */

    Search->pvLength[Node.ply] = Node.ply; 
    
    int king_square = (Game->side_to_move == SIDE_WHITE) ? Game->white_king_square : Game->black_king_square;
    int king_rank = king_square / 8;
    int king_file = king_square % 8;

    if (!is_king_safe(Game->board, king_rank, king_file, Game->side_to_move)) {
        return -MATE_SCORE +  (Node.ply);
    }
    return DRAW_SCORE;

}


static inline void play_null_move(struct GameState* Game, struct UndoInfo* Undo) {

    Undo->castling_rights = Game->castling_rights;
    Undo->en_passant_square = Game->en_passant_square;
    Undo->white_king_square = Game->white_king_square;
    Undo->black_king_square = Game->black_king_square;
    Undo->halfmove_clock = Game->halfmove_clock;
    Undo->zobrist_hash = Game->zobrist_hash;

    uint64_t hash = Undo->zobrist_hash;
    Game->zobrist_hash_3fold_history[Game->ply] = hash;

    if (Game->en_passant_square != -1) {
        hash ^= zobrist_en_passant_file[Game->en_passant_square & 7];
        Game->en_passant_square = -1;
    }

    Game->halfmove_clock++;

    // 4. Swap side to move, update hash, increment ply
    Game->side_to_move = !Game->side_to_move;
    hash ^= zobrist_side_to_move;
    Game->zobrist_hash = hash;
    (Game->ply)++;
}

static inline void unplay_null_move(struct GameState* Game, const struct UndoInfo* Undo) {
    (Game->ply)--;
    Game->side_to_move = !Game->side_to_move;

    // Restore exact state saved before the pass
    Game->en_passant_square = Undo->en_passant_square;
    Game->halfmove_clock    = Undo->halfmove_clock;
    Game->zobrist_hash      = Undo->zobrist_hash;
}


static inline int null_prune(struct GameState* Game, struct NodeState Node, struct SearchContext* Search, int* null_eval, int alpha, int beta) {

    int R = 2;
    
    int king_square = (Game->side_to_move == SIDE_WHITE) ? Game->white_king_square : Game->black_king_square;
    int king_rank = king_square / 8;
    int king_file = king_square % 8;

    if (Node.depth >= 3 && is_king_safe(Game->board, king_rank, king_file, Game->side_to_move)) {
    
        struct UndoInfo ui;
        play_null_move(Game, &ui);

        struct NodeState null_node = { .ply = Node.ply + 1, .depth = Node.depth - 1 - R, .null_node = 1 };
        *null_eval = -negamax(Game, Search, null_node, -beta, -beta + 1);

        unplay_null_move(Game, &ui);

        if (*null_eval >= beta) {
            *null_eval = beta;
            return 1;
        }
    }
    return 0;

}

static inline void check_search_time(struct SearchContext* Search) {

    const int TIME_CHECK_INTERVAL = 4096;
    if (Search->time_limit_ms && (Search->nodes % TIME_CHECK_INTERVAL) == 0) {
            clock_gettime(CLOCK_MONOTONIC, &Search->now);

            double time_taken_ms =
                (double)(Search->now.tv_sec - Search->start.tv_sec) * 1000.0 +
                (double)(Search->now.tv_nsec - Search->start.tv_nsec) / 1e6;

            if (time_taken_ms >= Search->time_limit_ms)
                Search->timed_out = 1;
    }
}

static int negamax(struct GameState* Game, struct SearchContext* Search, struct NodeState Node, int alpha, int beta) {

    if (is_game_a_draw(Game)) return DRAW_SCORE;
    
    if (Node.depth == 0)  return quiescence_search(Game, Node, Search, alpha, beta, 3);


    check_search_time(Search);
    if (Search->timed_out)  return 0;

    (Search->es.nodes)++;
    (Search->nodes)++;
    

    const int initial_alpha = alpha;
    const int initial_beta = beta;

    int best = NEGATIVE_INFINITY;
    int branch_eval;
    Move best_move = 0; 

    Move hash_move = 0;
    int tt_eval;
    int tt_cutoff = probe_tt(&hash_move, alpha, beta, &tt_eval, Search, Node, Game);
    if (tt_cutoff) {
        Search->pvLength[Node.ply] = Node.ply;
        return tt_eval;
    }

   int null_eval;
   if (!Node.null_node && null_prune(Game, Node, Search, &null_eval, alpha, beta)) return null_eval;

    total_generate_moves++;
    Move legal_moves[256];
    int move_count = generate_legal_moves(Game, legal_moves);

    if (move_count == 0) return handle_no_legal_moves(Game, Search, Node);

    int move_scores[256] = {0};
    evaluate_moves(Game, legal_moves, move_count, Node, move_scores, Search, hash_move); 

    for (int k = 0; k < move_count; k++) {

        Search->pvLength[Node.ply + 1] = Node.ply + 1;
    
        pick_best_move_first(move_scores, legal_moves, move_count, k);
        Move move = legal_moves[k];

        struct UndoInfo ui;
        make_move(move, Game, &ui, &Search->eb); 
                                                              
        struct NodeState child_node = { .ply = Node.ply + 1, .depth = Node.depth - 1 };
        branch_eval = -negamax(Game, Search, child_node, -beta, -alpha);

        unmake_move(move, Game, &ui);


        update_best(&best, &best_move, move, branch_eval);

        update_alpha_and_pv(&Node, &alpha, Search, branch_eval, move);

        if (is_alpha_beta_cutoff(alpha, beta)) {
            record_cutoff(Node, Search, move, hash_move);
            break;
        }
    }


    store_position_tt(Node, Game, best_move, best, initial_alpha, initial_beta);

    return best;
}

static inline void half_array(int array[64][64]) {

    /* Half the entries in the array (currently unused for history) */

    for (int to = 0; to < 64; to++) {
        for (int from = 0; from < 64; from++) {
            array[from][to] = (array[from][to] >> 1);
        }
    }
}

static inline void sort_rootmoves(struct RootMove root_legal_moves[256], int move_count) {

    /* Sort the root moves in decending evaluation */

    struct RootMove movecopy = {0};
    for (int moves = 1; moves < move_count; moves++) {
        movecopy = root_legal_moves[moves];
        int j = moves - 1;
        while (j >= 0 && movecopy.eval > root_legal_moves[j].eval) {
            root_legal_moves[j+1] = root_legal_moves[j]; 
            j--;
        }
        root_legal_moves[j + 1] = movecopy;
    }
}


Move iterative_deepening(struct GameState Game, int max_depth, int max_time) {

     /*
     Finds the optimal move at a certain depth,
    using negamax and iterative deepening.
    */


    struct SearchContext Search = {0};
    Search.eb = defEvalboards();
    initiate_evaluation(&Game, &Search.eb);
    

    Move legal_moves[256];
    int move_count = generate_legal_moves(&Game, legal_moves);
    
    struct RootMove root_legal_moves[256];
    for (int i = 0; i < move_count; i++) {
        root_legal_moves[i].Move = legal_moves[i];
        root_legal_moves[i].eval = NEGATIVE_INFINITY;
    }
    

    clock_gettime(CLOCK_MONOTONIC, &Search.start);

    Search.time_limit_ms = (max_time > 0) ? max_time : 0;
    max_depth = (Search.time_limit_ms > 0) ? 30 : max_depth;


    Move best_move = 0;

    for (int iteration_depth = 1; iteration_depth <= max_depth; iteration_depth++) { 

        // Used for estimating time remaining
        int previous_nodes = Search.nodes;
        double bf = 1;

        Search.pvLength[1] = 1;

        struct NodeState Node = { .ply = 0, .depth = iteration_depth };
        int alpha = NEGATIVE_INFINITY;
        int beta  = POSITIVE_INFINITY;
 
        for (int k = 0; k < move_count; k++) {


            struct UndoInfo ui;
            make_move(root_legal_moves[k].Move, &Game, &ui, &Search.eb);

            struct NodeState child_node = { .ply = Node.ply + 1, .depth = Node.depth - 1 };
            int branch_eval = -negamax(&Game, &Search, child_node, -beta, -alpha);

            root_legal_moves[k].eval = branch_eval;

            unmake_move(root_legal_moves[k].Move, &Game, &ui);

            if (Search.timed_out) return best_move;

            update_alpha_and_pv(&Node, &alpha, &Search, branch_eval, root_legal_moves[k].Move);

        }

        sort_rootmoves(root_legal_moves, move_count);
        best_move = root_legal_moves[0].Move;

        //half_array(Search.history_table);

        clock_gettime(CLOCK_MONOTONIC, &Search.now);
        double time_taken =
            (Search.now.tv_sec - Search.start.tv_sec) +
            (Search.now.tv_nsec - Search.start.tv_nsec) / 1e9;

        int who2play = (Game.side_to_move == SIDE_WHITE) ? 1 : -1;
        write_info(root_legal_moves[0].eval * who2play, iteration_depth, Search.nodes, time_taken, Search.pvLength[0], Search.pvTable);

        if (Search.time_limit_ms) {
            if (previous_nodes) bf = (double)Search.nodes/previous_nodes;
            if (1.2 * time_taken * bf * 1000 > Search.time_limit_ms) {
                break;
            }
        }
    }


    return best_move;
}

