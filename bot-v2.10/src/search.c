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


static inline struct SearchResult tt_unsafe_draw_result(void) {
    return (struct SearchResult){ 0, 1 };
}


static inline int is_game_a_draw(struct GameState* Game) {

    /* returns true if the game is a draw due to repetition or halfmove clock */

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

static inline struct NodeState create_child_node(struct NodeState Node, struct SearchContext* Search, Move move) {

    /* Prepares a child node based on the current node state */

    struct NodeState child;
    child.alpha = -Node.beta;
    child.beta = -Node.alpha;
    child.ply = Node.ply + 1;
    child.depth = Node.depth - 1;
    child.last_played_move = move;
    child.null = 0;
    if (Node.on_pv && Node.ply < Search->old_pv_length && moves_match(move, Search->old_pv_line[Node.ply])) {
        child.on_pv = 1;
    }
    else {
        child.on_pv = 0;
    }
    return child;
}



static struct SearchResult quiescence_search(struct GameState* Game, struct NodeState Node, struct SearchContext* Search, int checkBuffer) {


    (Search->es.qnodes)++;
    if (is_game_a_draw(Game)) return tt_unsafe_draw_result();

    // Initialize
    int total_legal_moves = 0;
    struct SearchResult branch_result = {0};
    int eval_order[256];
    Move legal_moves[256];

    // Evaluate if the king is safe
    int inCheck = 0;
    
    /*
    int king_square = (Game->side_to_move == SIDE_WHITE) ? Game->white_king_square : Game->black_king_square;
    int king_rank = king_square / 8;
    int king_file = king_square % 8;
    
    if (!is_king_safe(Game->board, king_rank, king_file, Game->side_to_move)) {
        inCheck = 1;
    }
    */
    
    

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
        if (StandPat >= Node.beta) {
            (Search->es.standpat_cutoff)++;
            branch_result.eval = Node.beta;
            return branch_result;
        }
        // If StandPat > alpha, we can garantee stand pat if not capture in overtime is made
        if (StandPat  > Node.alpha) {
            Node.alpha = StandPat;
        }
    }

    // Store best value
    struct SearchResult best_result = {0};
    best_result.eval = StandPat;

    // If not in check, find only captures
    if (!inCheck) {
        total_legal_moves = generate_legal_captures(Game, legal_moves);
        sort_moves_quiescence(Game, legal_moves, total_legal_moves, Node.last_played_move, eval_order, Search->history_table); 
    }
    // In check find all legal evasions 
    if (inCheck) {
        total_legal_moves = generate_legal_moves(Game, legal_moves);
        sort_moves_quiescence(Game, legal_moves, total_legal_moves, Node.last_played_move, eval_order, Search->history_table); 
    }
    for (int k = 0; k < total_legal_moves; k++) {

        // eval_order given by sort_moves()
        Move move = legal_moves[eval_order[k]];

        struct UndoInfo ui;
        make_move(move, Game, &ui, &Search->eb); 

        struct NodeState child_node = create_child_node(Node, Search, move);
        branch_result = quiescence_search(Game, child_node, Search, checkBuffer);
        branch_result.eval = -branch_result.eval;

        // Evaluate alpha beta values
        if (branch_result.eval > best_result.eval) {
            best_result = branch_result;
        }
        if (branch_result.eval > Node.alpha) {
            Node.alpha = branch_result.eval;
        }
        // Break fi alpha beta condition is reached
        if (Node.beta <= Node.alpha) {
            if (get_capture_flag(move) == 1) {
                (Search->es.capture_cutoff)++;
            }
            else {
                (Search->es.quiet_cutoff)++;
            }
            unmake_move(move, Game, &ui);
            (Search->es.qcutoffmove[k])++;
            break;
        }
        unmake_move(move, Game, &ui);

    }



    // If not legal moves - Return basecase or checkmate
    if (total_legal_moves == 0) {
        struct SearchResult end_result = {0};
        // If in check we have found all legal moves due to check evasion. So we can garantee its mate
        if (inCheck) {
            (Search->es.qcheckmate)++;
            end_result.eval = -MATE_SCORE + Node.ply;
            return end_result;
        }
        else { // We dont know if its stalemate or if simply no captures exist
            end_result.eval = StandPat;
            return end_result;
        }
    }

    return best_result;
} 

static inline int probe_tt(Move* hash_move, struct SearchResult* return_result, struct SearchContext* Search, struct NodeState Node, struct GameState* Game) {

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

    if (tt_age == 0 && entry->depth >= Node.depth && !Node.on_pv) {


        int16_t entry_eval = entry->eval;
        if (entry_eval >= MATE_IN_100_SCORE) entry_eval -= Node.ply;
        if (entry_eval <= -MATE_IN_100_SCORE) entry_eval += Node.ply;

        if (entry->flag == EXACT){
            return_result->eval = entry_eval;
            return 1;
        }

        if (entry->flag == UPPERBOUND) {
            if (entry_eval <= Node.alpha) {
                return_result->eval = entry_eval;
                return 1;
            }
        }
        if (entry->flag == LOWERBOUND) {
            if (entry_eval >= Node.beta) {
                return_result->eval = entry_eval;
                return 1;
            }
        }
    }
    return 0;

}

static inline void store_position_tt(struct NodeState Node, struct GameState* Game, Move best_move, struct SearchResult best, int old_alpha, int old_beta) {

    /* Stores the current position in the tt */


    if (best.tt_unsafe == 1) return;

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

        int16_t entry_eval = best.eval;

        if (entry_eval >= MATE_IN_100_SCORE)
            entry_eval += Node.ply;

        if (entry_eval <= -MATE_IN_100_SCORE)
            entry_eval -= Node.ply;

        entry->eval = entry_eval;

        if (best.eval <= old_alpha) {
            entry->flag = UPPERBOUND;
        }
        else if (best.eval >= old_beta) {
            entry->flag = LOWERBOUND;
        }
        else {
            entry->flag = EXACT;
        }
    }
}

static inline void update_best(struct SearchResult *best_result, Move* best_move, Move move, struct SearchResult branch_result) {

    /* Updates the best value if beaten */

    if (branch_result.eval > best_result->eval) {
        *best_move = move;
        *best_result = branch_result;
    }
}

static inline void update_alpha_and_pv(struct NodeState* Node, struct SearchContext* Search, struct SearchResult best_result, Move move) {

    /* Updates alpha if beaten, also adjusting pv line */

    if (best_result.eval > Node->alpha) {
        Node->alpha = best_result.eval;

        // Update pv
        Search->pvTable[Node->ply][Node->ply] = move;

        // Copy child line
        for (int i = Node->ply + 1; i < Search->pvLength[Node->ply + 1]; i++) {
            Search->pvTable[Node->ply][i] = Search->pvTable[Node->ply + 1][i];
        }
        Search->pvLength[Node->ply] = Search->pvLength[Node->ply + 1];
    }
}

static inline int is_alpha_beta_cutoff(struct NodeState Node) {

    return Node.beta <= Node.alpha;
}

static inline void record_cutoff(struct NodeState Node, struct SearchContext* Search, Move move, Move hash_move) {

    /* Updates heuristics based on cutoff */

    if (moves_match(move, hash_move)) (Search->es.hash_move_cutoff)++;

    if (get_capture_flag(move) == QUIET) {

        Search->killer_moves[Node.ply][1] = Search->killer_moves[Node.ply][0];
        Search->killer_moves[Node.ply][0] = move;

        int ss = get_from_square(move);
        int ts = get_to_square(move);
        Search->history_table[ss][ts] += (Node.depth * Node.depth + 5) / 5;
        if (Search->history_table[ss][ts] > 999999) {
            Search->history_table[ss][ts] = 999999;
        }

        (Search->es.quiet_cutoff)++;
    }
    else {
        (Search->es.capture_cutoff)++;
    }
}


static inline struct SearchResult handle_no_legal_moves(struct GameState* Game, struct SearchContext* Search, struct NodeState Node) {

    /* Evaluate if check mate, else defualt Stalemate */

    struct SearchResult end_result = {0};

    Search->pvLength[Node.ply] = Node.ply; 
    
    int king_square = (Game->side_to_move == SIDE_WHITE) ? Game->white_king_square : Game->black_king_square;
    int king_rank = king_square / 8;
    int king_file = king_square % 8;

    if (!is_king_safe(Game->board, king_rank, king_file, Game->side_to_move)) {
        (Search->es.checkmate)++;
        end_result.eval = -MATE_SCORE +  (Node.ply);
    }
    (Search->es.stalemate)++;
    return end_result;

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

static inline struct NodeState create_null_node(struct NodeState Node) {

    /* Prepares a child null node based on the current node state */

    int R = 2; // Node reduction

    struct NodeState child = {0};
    child.alpha = -Node.beta;
    child.beta = -Node.beta + 1;
    child.ply = Node.ply + 1;
    child.depth = Node.depth - 1 - R;
    child.null = 1;
    return child;
}

static inline int has_non_pawn_material(struct GameState* Game) {
    int phase = (Game->side_to_move == SIDE_WHITE) ? Game->eval.white_phase : Game->eval.black_phase;
    return phase;
}
static inline int null_prune(struct GameState* Game, struct NodeState Node, struct SearchContext* Search, struct SearchResult* null_result) {
    
    int king_square = (Game->side_to_move == SIDE_WHITE) ? Game->white_king_square : Game->black_king_square;
    int king_rank = king_square / 8;
    int king_file = king_square % 8;

    if (!Node.on_pv && Node.depth >= 3 && has_non_pawn_material(Game) && is_king_safe(Game->board, king_rank, king_file, Game->side_to_move)) {
    
        struct UndoInfo ui;
        play_null_move(Game, &ui);

        struct NodeState null_node = create_null_node(Node);
        *null_result = branch(Game, null_node, Search);
        null_result->eval = -null_result->eval;

        unplay_null_move(Game, &ui);

        if (null_result->eval >= Node.beta) {
            null_result->eval = Node.beta;
            return 1;
        }
    }
    return 0;

}

static struct SearchResult branch(struct GameState* Game, struct NodeState Node, struct SearchContext* Search) {

    if (is_game_a_draw(Game)) return tt_unsafe_draw_result();
    
    if (Node.depth == 0) {
        Node.on_pv = 0;
        return quiescence_search(Game, Node, Search, 3);
    }

    (Search->es.nodes)++;

    const int initial_alpha = Node.alpha;
    const int initial_beta = Node.beta;

    struct SearchResult best_result = {0};
    best_result.eval = NEGATIVE_INFINITY;

    struct SearchResult branch_result = {0};
    Move best_move = {0};


    Move hash_move = {0};
    struct SearchResult tt_result = {0};
    int tt_cutoff = probe_tt(&hash_move, &tt_result, Search, Node, Game);
    if (tt_cutoff) {
        Search->pvLength[Node.ply] = Node.ply;
        return tt_result;
    }

    if (Node.null != 1) {
        struct SearchResult null_result = {0};
        if (null_prune(Game, Node, Search, &null_result)) return null_result;
    }

    total_generate_moves++;
    Move legal_moves[256];
    int move_count = generate_legal_moves(Game, legal_moves);

    int move_scores[256] = {0};
    evaluate_moves(Game, legal_moves, move_count, Node, move_scores, Search, hash_move); 

    for (int k = 0; k < move_count; k++) {

        Search->pvLength[Node.ply + 1] = Node.ply + 1;
    
        pick_best_move_first(move_scores, legal_moves, move_count, k);
        Move move = legal_moves[k];

        struct UndoInfo ui;
        make_move(move, Game, &ui, &Search->eb); 
                                                              
        struct NodeState child_node = create_child_node(Node, Search, move);
        branch_result = branch(Game, child_node, Search);
        branch_result.eval = -branch_result.eval;

        unmake_move(move, Game, &ui);

        update_best(&best_result, &best_move, move, branch_result);

        update_alpha_and_pv(&Node, Search, branch_result, move);

        if (is_alpha_beta_cutoff(Node)) {
            record_cutoff(Node, Search, move, hash_move);
            break;
        }
    }

    if (move_count == 0) {
        return handle_no_legal_moves(Game, Search, Node);
    }

    store_position_tt(Node, Game, best_move, best_result, initial_alpha, initial_beta);

    return best_result;
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

static inline void refactor_old_pv_line(struct SearchContext* Search) {

    /* Store the old pv line */

    Search->old_pv_length = Search->pvLength[0];
    for (int i = 0; i < Search->pvLength[0]; i++) {
        Search->old_pv_line[i] = Search->pvTable[0][i];
    }
}

Move iterative_deepening(struct GameState Game, int max_depth, int max_time) {

     /*
     Finds the optimal move at a certain depth,
    using negamax and iterative deepening.
    */

    if (max_time != -1) max_depth = 20;

    struct SearchContext Search = {0};
    Search.eb = defEvalboards();
    initiate_evaluation(&Game, &Search.eb);

    

    // Find all legal moves 
    Move legal_moves[256];
    int move_count = generate_legal_moves(&Game, legal_moves);
    

    // Turn into RootMoves conatining move and eval for sorting
    struct RootMove root_legal_moves[256] = {0};
    for (int i = 0; i < move_count; i++) {
        root_legal_moves[i].Move = legal_moves[i];
        root_legal_moves[i].eval = NEGATIVE_INFINITY;
    }
    

    // Start timer and reset node count
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    uint64_t total_nodes = 0;

    
    for (int iterationdepth = 1; iterationdepth <= max_depth; iterationdepth++) { 

        // Used for estimating time remaining
        int previous_nodes = total_nodes;
        double bf = 1;
        Search.es.nodes = 0;
        Search.es.qnodes = 0;

        Search.pvLength[1] = 1;

        struct NodeState Node = {0};
        Node.alpha = NEGATIVE_INFINITY;
        Node.beta  = POSITIVE_INFINITY;
        Node.ply = 0;
        Node.depth = iterationdepth;
        Node.on_pv = 1;
 
        // Iterate over legalmoves
        for (int k = 0; k < move_count; k++) {

            struct UndoInfo ui;
            make_move(root_legal_moves[k].Move, &Game, &ui, &Search.eb);

            struct NodeState child_node = create_child_node(Node, &Search, root_legal_moves[k].Move);
            struct SearchResult branch_result = branch(&Game, child_node, &Search);
            branch_result.eval = -branch_result.eval;

            root_legal_moves[k].eval = branch_result.eval;

            unmake_move(root_legal_moves[k].Move, &Game, &ui);

            update_alpha_and_pv(&Node, &Search, branch_result, root_legal_moves[k].Move);

        }

        sort_rootmoves(root_legal_moves, move_count);

        refactor_old_pv_line(&Search);
        //half_array(Search.history_table);

        // End timer
        clock_gettime(CLOCK_MONOTONIC, &now);
        double time_taken =
            (now.tv_sec - start.tv_sec) +
            (now.tv_nsec - start.tv_nsec) / 1e9;

        total_nodes += Search.es.qnodes + Search.es.nodes;
        int who2play = (Game.side_to_move == SIDE_WHITE) ? 1 : -1;
        write_info(root_legal_moves[0].eval * who2play, iterationdepth, total_nodes, time_taken, Search.pvLength[0], Search.pvTable);

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

