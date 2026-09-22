#include <stdlib.h>
#include "types.h"
#include <stdio.h>
#include <time.h>

#include "search.h"
#include "movegen.h"
#include "playmove.h"
#include "unplaymove.h"
#include "eval.h"
#include "sortmoves.h"
#include "uci.h"
#include "tt.h"
#include "zobrist.h"
#include <string.h>
#include "params.h"
#include <assert.h>
#include "board.h"
#include <math.h>

atomic_bool stop_search = false;

int all_node = 0;
int pv_nodes = 0;
int cut_node = 0;
int cut_node_early = 0;


static uint8_t lmr_table[MAX_DEPTH][MAX_MOVES];

void init_lmr_table(void) {
    for (int depth = 0; depth < MAX_DEPTH; depth++) {
        for (int move = 0; move < MAX_MOVES; move++) {

            int reduction = LMR_BASE + LMR_MULTIPLIER * log(depth) * log(move) / LMR_DIVISOR;

            if (reduction < 0)
                reduction = 0;

            lmr_table[depth][move] = (uint8_t)reduction;
        }
    }
}


static inline int is_game_a_draw(struct GameState* Game) {

    /* returns true if the game is a draw due to repetition of halfmove clock */

    if (Game->halfmove_clock >= 100) return 1;

    int limit = (Game->halfmove_clock < Game->ply) ? Game->halfmove_clock : Game->ply;

    for (int old_ply = 2; old_ply <= limit; old_ply += 2) { // Repetition
        if (Game->zobrist_hash == Game->zobrist_hash_3fold_history[Game->ply - old_ply]) return 1;
    }

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
    Move temp_move = legal_moves[k];
    int temp_eval = eval[k];
    eval[k] = eval[max_arg];
    eval[max_arg] = temp_eval;
    legal_moves[k] = legal_moves[max_arg];
    legal_moves[max_arg] = temp_move;
}


static inline int is_ray_clear(int b[8][8], int r1, int f1, int r2, int f2, int dr, int df) {
    int r = r1 + dr;
    int f = f1 + df;
    while (r != r2 || f != f2) {

        if (b[r][f] != EMPTY) return 0; 
        r += dr;
        f += df;
    }
    return 1;
}

static inline int piece_gives_check(struct GameState* Game, Move move, int king_rank, int king_file) {

    /* Evaluates if the moved piece gives check */

    int to_sq = get_to_square(move);

    int to_r = to_sq / 8;
    int to_f = to_sq % 8;

    int piece_type = abs(Game->board[to_r][to_f]);

    int dr = king_rank - to_r;
    int df = king_file - to_f;
    int abs_dr = abs(dr);
    int abs_df = abs(df);

    switch (piece_type) {
        case PAWN: {
            int p_step = (Game->side_to_move == SIDE_WHITE) ? -1 : 1;
            return (dr == p_step && abs_df == 1);
        }
        case KNIGHT:
            return (abs_dr == 1 && abs_df == 2) || (abs_dr == 2 && abs_df == 1);

        case BISHOP:
            if (abs_dr == abs_df && abs_dr > 0) {
                int step_r = (dr > 0) ? 1 : -1;
                int step_f = (df > 0) ? 1 : -1;
                return is_ray_clear(Game->board, to_r, to_f, king_rank, king_file, step_r, step_f);
            }
            return 0;

        case ROOK:
            if ((abs_dr == 0 && abs_df > 0) || (abs_df == 0 && abs_dr > 0)) {
                int step_r = (dr == 0) ? 0 : ((dr > 0) ? 1 : -1);
                int step_f = (df == 0) ? 0 : ((df > 0) ? 1 : -1);
                return is_ray_clear(Game->board, to_r, to_f, king_rank, king_file, step_r, step_f);
            }
            return 0;

        case QUEEN:
            if (abs_dr == abs_df || abs_dr == 0 || abs_df == 0) {
                int step_r = (dr == 0) ? 0 : ((dr > 0) ? 1 : -1);
                int step_f = (df == 0) ? 0 : ((df > 0) ? 1 : -1);
                return is_ray_clear(Game->board, to_r, to_f, king_rank, king_file, step_r, step_f);
            }
            return 0;

        default:
            return 0;
    }
}

static inline int sign(int val) {
    return (val > 0) - (val < 0);
}

static inline int is_ray_giving_check(struct GameState* Game, int k_r, int k_f, int dr, int df, int king_colour) {
    int r = k_r + dr;
    int f = k_f + df;

        
    while (r >= 0 && r < 8 && f >= 0 && f < 8 && Game->board[r][f] == EMPTY) {
        r += dr;
        f += df;
    }

    // Hit edge of board
    if (r < 0 || r >= 8 || f < 0 || f >= 8) return 0;

    int piece = Game->board[r][f];

    int is_enemy = (king_colour == SIDE_WHITE) ? (piece < 0) : (piece > 0);
    if (!is_enemy) return 0;

    int piece_type = abs(piece);

    if (dr == 0 || df == 0) {
        return (piece_type == ROOK || piece_type == QUEEN);
    }
    if (abs(dr) == 1 && abs(df) == 1) {
        return (piece_type == BISHOP || piece_type == QUEEN);
    }

    return 0;
}

static inline int piece_discovers_check(struct GameState* Game, Move move, int king_rank, int king_file) {

    /* Evaluates if a move unleashes a discoverd check */

    int from_sq = get_from_square(move);
    int from_r = from_sq / 8;
    int from_f = from_sq % 8;

    int d_rank = from_r - king_rank;
    int d_file = from_f - king_file;

    
    if (d_rank == 0 || d_file == 0) {
        int dr = sign(d_rank);
        int df = sign(d_file);
        return is_ray_giving_check(Game, king_rank, king_file, dr, df, Game->side_to_move);
    }

    if (abs(d_rank) == abs(d_file)) {
        int dr = sign(d_rank);
        int df = sign(d_file);
        return is_ray_giving_check(Game, king_rank, king_file, dr, df, Game->side_to_move);
    }

    return 0;
}

static inline int move_gives_check(struct GameState* Game, Move move) {


    int king_square = (Game->side_to_move == SIDE_WHITE) ? Game->white_king_square : Game->black_king_square;
    int king_rank = king_square / 8;
    int king_file = king_square % 8;

    int move_type = get_move_type(move);
    if (move_type == EN_PASSANT || move_type == SHORT_CASTLE || move_type == LONG_CASTLE) return !is_king_safe(Game->board, king_rank, king_file, Game->side_to_move);

    if (piece_gives_check(Game, move, king_rank, king_file)) return 1;
    if (piece_discovers_check(Game, move, king_rank, king_file)) return 1;

    return 0;
}

static inline void update_sel_depth(struct SearchContext* Search, struct NodeState Node) {
    if (Node.ply > Search->max_sel_depth) Search->max_sel_depth = Node.ply;
}


static int quiescence_search(struct GameState* Game, struct NodeState Node, struct SearchContext* Search, int alpha, int beta, int checkBuffer) {


    (Search->es.qnodes)++;
    (Search->nodes)++;
    if (is_game_a_draw(Game)) return 0;
    update_sel_depth(Search, Node);

    // Initialize
    int total_legal_moves = 0;
    int branchEval = 0;
    int eval_order[256];
    Move legal_moves[256];

    // Evaluate if the king is safe
    int inCheck = Node.in_check;

    // Only allow so many check exstensions  
    if (inCheck) {
        if (checkBuffer <= 0) {
            //return evaluate_position(Game);
        }
        checkBuffer--; // Consume one check from the buffer
    }

    // Only allow StandPat if King is safe, else capture/move needs to be played
    int StandPat = NEGATIVE_INFINITY;
    if (!inCheck) {
        //int old_StandPat = Eval(Game, &Search->eb);

        StandPat = evaluate_position(Game);
        //assert(abs(old_StandPat - StandPat) <= 3);

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
        make_move(move, Game, &ui); 

        int new_move_check = move_gives_check(Game, move);

        struct NodeState child_node = { .ply = Node.ply + 1, .depth = Node.depth - 1, .in_check = new_move_check };
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
        Search->pvLength[Node.ply] = Node.ply; 
        // If in check we have found all legal moves due to check evasion. So we can garantee its mate
        if (inCheck) {
            (Search->es.qcheckmate)++;
            return -MATE_SCORE +  (Node.ply);
        }
        else { // We dont know if its stalemate or if simply no captures exist
            return StandPat;
        }
    }

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

    if ((tt_age == 0) && entry->depth >= Node.depth) {

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


    if (tt_age > 0 || Node.depth >= entry->depth) { // Works as a replacement strategy, but could need more work

        entry->zobrist_key = Game->zobrist_hash;
        entry->best_move = best_move;
        entry->depth = Node.depth;
        entry->generation = tt_generation;

        int entry_eval = best;

        // Adjust for mate
        if (entry_eval >= MATE_IN_100_SCORE)
            entry_eval += Node.ply;

        if (entry_eval <= -MATE_IN_100_SCORE)
            entry_eval -= Node.ply;

        entry->eval = entry_eval;

        // Store with correct flag
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

static inline void cutoff_updates(struct NodeState Node, struct SearchContext* Search, Move move) {

    /* Updates move heuristics based on cutoff */

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

    Undo->en_passant_square = Game->en_passant_square;
    Undo->halfmove_clock = Game->halfmove_clock;
    Undo->zobrist_hash = Game->zobrist_hash;

    uint64_t hash = Undo->zobrist_hash;
    Game->zobrist_hash_3fold_history[Game->ply] = hash;

    if (Game->en_passant_square != -1) {
        hash ^= zobrist_en_passant_file[Game->en_passant_square & 7];
        Game->en_passant_square = -1;
    }

    Game->halfmove_clock++;
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


static inline int has_non_pawn_material(struct GameState* Game) {
    int phase = (Game->side_to_move == SIDE_WHITE) ? Game->eval.white_phase : Game->eval.black_phase;
    return phase;
}

static inline int null_prune(struct GameState* Game, struct NodeState Node, struct SearchContext* Search, int* null_eval, int beta, int pv_node) {

    if (Node.null_node) return 0;

    static const int R = NMP_BASE_REDUCTION;
    
    if (!pv_node && Node.depth >= NMP_MIN_DEPTH && has_non_pawn_material(Game) && !Node.in_check) {
    
        struct UndoInfo ui;
        play_null_move(Game, &ui);

        struct NodeState null_node = { .ply = Node.ply + 1, .depth = Node.depth - 1 - R, .null_node = 1, .in_check = 0};
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

    if (Search->node_limit && Search->nodes >= Search->node_limit) Search->timed_out = 1;

    if ((Search->nodes % TIME_CHECK_INTERVAL) == 0) {

        if (atomic_load_explicit(&stop_search, memory_order_relaxed)) {
            Search->timed_out = 1;
        }

        if (Search->time_limit_ms) {
            clock_gettime(CLOCK_MONOTONIC, &Search->now);

            double time_taken_ms =
                (double)(Search->now.tv_sec - Search->start.tv_sec) * 1000.0 +
                (double)(Search->now.tv_nsec - Search->start.tv_nsec) / 1e6;

            if (time_taken_ms >= Search->time_limit_ms)
                Search->timed_out = 1;
        }
    }
}

int negamax(struct GameState* Game, struct SearchContext* Search, struct NodeState Node, int alpha, int beta) {

    if (is_game_a_draw(Game)) return DRAW_SCORE;
    
    if (Node.depth <= 0)  return quiescence_search(Game, Node, Search, alpha, beta, 3);
    update_sel_depth(Search, Node);

    check_search_time(Search);
    if (Search->timed_out)  return 0;

    (Search->es.nodes)++;
    (Search->nodes)++;


    int pv_node = (beta - alpha > 1);

    Move hash_move = 0;
    int tt_eval;
    int tt_cutoff = probe_tt(&hash_move, alpha, beta, &tt_eval, Search, Node, Game);
    if (tt_cutoff) {
        Search->pvLength[Node.ply] = Node.ply;
        return tt_eval;
    }

    int null_eval;
    int null_cutoff = null_prune(Game, Node, Search, &null_eval, beta, pv_node);
    if (null_cutoff) return null_eval;


    const int initial_alpha = alpha;
    const int initial_beta = beta;

    int best = NEGATIVE_INFINITY;
    int branch_eval;
    Move best_move = 0; 

    Move legal_moves[256];
    int move_count = generate_legal_moves(Game, legal_moves);

    if (move_count == 0) return handle_no_legal_moves(Game, Search, Node);

    int move_scores[256] = {0};
    evaluate_moves(Game, legal_moves, move_count, Node, move_scores, Search, hash_move); 

    int moves_searched = 0;
    for (int k = 0; k < move_count; k++) {

        moves_searched++;
        Search->pvLength[Node.ply + 1] = Node.ply + 1;
    
        pick_best_move_first(move_scores, legal_moves, move_count, k);
        Move move = legal_moves[k];

        struct UndoInfo ui;
        make_move(move, Game, &ui); 
                                                              
        int move_check = move_gives_check(Game, move);

        if (pv_node && k == 0) {
            struct NodeState child_node = { .ply = Node.ply + 1, .depth = Node.depth - 1, .in_check = move_check };
            branch_eval = -negamax(Game, Search, child_node, -beta, -alpha);
        }
        else {


            int reduction = 0;
            /*
            if (k > 4 && Node.depth >= 5 && get_capture_flag(move) == QUIET && !move_check) {
                reduction = 2;
                if (k > 10 && Node.depth >= 7) reduction = 3;
            }
            */

            if (k > LMR_MIN_MOVE && Node.depth >= LMR_MIN_DEPTH && get_capture_flag(move) == QUIET && !move_check) {
                reduction = lmr_table[Node.depth][k];
            }

            struct NodeState child_node = { .ply = Node.ply + 1, .depth = Node.depth - 1 - reduction, .in_check = move_check};
            branch_eval = -negamax(Game, Search, child_node, -(alpha + 1), -alpha);

            if (reduction > 0 && branch_eval > alpha) {
                child_node.depth = Node.depth - 1;
                branch_eval = -negamax(Game, Search, child_node, -(alpha + 1), -alpha);
            }

            if (pv_node && branch_eval > alpha && branch_eval < beta) {
                branch_eval = -negamax(Game, Search, child_node, -beta, -alpha);
            }
        }



        unmake_move(move, Game, &ui);

        update_best(&best, &best_move, move, branch_eval);

        update_alpha_and_pv(&Node, &alpha, Search, branch_eval, move);

        if (is_alpha_beta_cutoff(alpha, beta)) {
            cutoff_updates(Node, Search, move);

            cut_node++;
            if (k < 5) cut_node_early ++;

            break;
        }
    }

    if (moves_searched == move_count) {
        if (!pv_node) all_node++;
        else pv_nodes++;
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

static inline void sort_rootmoves(struct RootMove root_moves[256], int move_count) {

    /* Sort the root moves in decending evaluation */

    struct RootMove movecopy = {0};
    for (int moves = 1; moves < move_count; moves++) {
        movecopy = root_moves[moves];
        int j = moves - 1;
        while (j >= 0 && movecopy.eval > root_moves[j].eval) {
            root_moves[j+1] = root_moves[j]; 
            j--;
        }
        root_moves[j + 1] = movecopy;
    }
}

static inline int search_root(struct GameState* Game, struct SearchContext* Search, struct NodeState Node, int alpha, int beta, struct RootMove root_moves[256], int move_count) {

    /* Starts the searching by calling negamax */

    int best_score = NEGATIVE_INFINITY;
    Move best_move = 0;
    int branch_eval;

    for (int k = 0; k < move_count; k++) {

        Move move = root_moves[k].Move;

        struct UndoInfo ui;
        make_move(move, Game, &ui);

        int new_move_check = move_gives_check(Game, move);
        struct NodeState child_node = { .ply = Node.ply + 1, .depth = Node.depth - 1, .in_check = new_move_check };


        if (k == 0) {
            branch_eval = -negamax(Game, Search, child_node, -beta, -alpha);
        }
        else {
            branch_eval = -negamax(Game, Search, child_node, -(alpha + 1), -alpha);

            if (branch_eval > alpha && branch_eval < beta) {
                branch_eval = -negamax(Game, Search, child_node, -beta, -alpha);
            }
        }
        
        root_moves[k].eval = branch_eval;

        unmake_move(move, Game, &ui);

        if (Search->timed_out) return best_score;

        update_best(&best_score, &best_move, move, branch_eval);

        update_alpha_and_pv(&Node, &alpha, Search, branch_eval, move);
    }

    return best_score;
}

Move iterative_deepening(struct GameState* Game, struct SearchContext* Search) {

    /* Runs the iteartive deepening loop, 
     finding and sorting root_moves for search_root to search */


    Move legal_moves[256];
    int move_count = generate_legal_moves(Game, legal_moves);

    if (move_count == 0) return 0;

    struct RootMove root_moves[256];
    for (int i = 0; i < move_count; i++) {
        root_moves[i].Move = legal_moves[i];
        root_moves[i].eval = NEGATIVE_INFINITY;
    }

    Move best_move = 0;
    int last_eval = NEGATIVE_INFINITY;

    for (int iteration_depth = 1; iteration_depth <= Search->max_depth; iteration_depth++) {

        // Used for estimating time remaining
        int previous_nodes = Search->nodes;
        double bf = 1;
        Search->max_sel_depth = 0;

        Search->pvLength[1] = 1;

        struct NodeState Node = { .ply = 0, .depth = iteration_depth };
        int alpha = NEGATIVE_INFINITY;
        int beta  = POSITIVE_INFINITY;

        if (iteration_depth > ASPIRATION_MIN_DEPTH) {
            int aspiration_delta = ASPIRATION_INITIAL_DELTA;
            alpha = last_eval - aspiration_delta;
            beta = last_eval + aspiration_delta;
        }

        while (1) { 

            int score = search_root(Game, Search, Node, alpha, beta, root_moves, move_count);

            if (Search->timed_out) return best_move;

            if (score <= alpha) {
                alpha = NEGATIVE_INFINITY; 
            } else if (score >= beta) {
                beta = POSITIVE_INFINITY;  
            } else {
                last_eval = score;        
                break;
            }
        }


        sort_rootmoves(root_moves, move_count);
        best_move = root_moves[0].Move;

        //half_array(Search->history_table);

        clock_gettime(CLOCK_MONOTONIC, &Search->now);
        double time_taken =
            (Search->now.tv_sec - Search->start.tv_sec) +
            (Search->now.tv_nsec - Search->start.tv_nsec) / 1e9;

        if (!Search->silent) write_info(root_moves[0].eval, iteration_depth, Search->max_sel_depth, Search->nodes, time_taken, Search->pvLength[0], Search->pvTable);

        if (Search->time_limit_ms) {
            if (previous_nodes) bf = (double)Search->nodes/previous_nodes;
            if (TIME_MARGIN * time_taken * bf * 1000 > Search->time_limit_ms) {
                break;
            }
        }
    }

    return best_move;
}

Move search_start(struct GameState Game, int depth, int time_left, int time_increment, int move_time, uint64_t node_limit) {

     /*
      Starts the search by defining time limits and max_depth
    */


    struct SearchContext Search = {0};
    initiate_evaluation(&Game);
    init_lmr_table();

    Search.time_limit_ms = (time_left > 0) ? time_left / TIME_REMAINING_DIVISOR + time_increment /TIME_INCRAMENT_DIVISOR : move_time;
    Search.max_depth = (depth == 0) ? ENGINE_MAX_DEPTH : depth;
    Search.node_limit = node_limit;
    

    clock_gettime(CLOCK_MONOTONIC, &Search.start);

    Move best_move = iterative_deepening(&Game, &Search);

    printf("Cut nodes: %d\n", cut_node);
    printf("Cut nodes at move < 5: %d\n", cut_node_early);
    printf("All nodes: %d\n", all_node);
    printf("Pv nodes: %d\n", pv_nodes);
    return best_move;
}

