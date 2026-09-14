#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>
#include <stdlib.h>

// enums
enum Side { SIDE_BLACK = 0, SIDE_WHITE = 1 };
enum Piece { EMPTY = 0, PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6 };
enum MoveType { STANDARD = 0, EN_PASSANT = 1, SHORT_CASTLE = 2, LONG_CASTLE = 3, PROMOTION = 4 };
enum CaptureFlag { QUIET = 0, CAPTURE = 1 };
enum CastlingRights { WK = 1, WQ = 2, BK = 4, BQ = 8 };
enum Shift { FROM_SHIFT = 0, TO_SHIFT = 6, PIECE_INDEX_SHIFT = 12, MOVE_TYPE_SHIFT = 16, CAPTURE_SHIFT = 19, PROMOTION_TYPE_SHIFT = 20, CAPTURED_INDEX_SHIFT = 23 } ;
enum Evaluations { DRAW_SCORE = 0, MATE_SCORE = 30000, MATE_IN_100_SCORE = 29900, NEGATIVE_INFINITY = -32000, POSITIVE_INFINITY = 32000 };

enum Mask { FROM_MASK = 63 << FROM_SHIFT, TO_MASK = 63 << TO_SHIFT, PIECE_INDEX_MASK = 15 << PIECE_INDEX_SHIFT, MOVE_TYPE_MASK = 7 << MOVE_TYPE_SHIFT, CAPTURE_MASK = 1 << CAPTURE_SHIFT, PROMOTION_TYPE_MASK = 7 << PROMOTION_TYPE_SHIFT, CAPTURED_INDEX_MASK = 15 << CAPTURED_INDEX_SHIFT , 
    MOVE_MASK = FROM_MASK | TO_MASK | MOVE_TYPE_MASK | CAPTURE_MASK | PROMOTION_TYPE_MASK
};

enum EVAL { BISHOP_PAIR_BONUS = 10, DOUBLED_PAWN_PENALTY = 5 };
typedef uint32_t Move;

static inline int moves_match(Move a, Move b) {
    if (!a || !b) return 0;
    return (a & MOVE_MASK) == (b & MOVE_MASK);
}

static inline int get_from_square(Move move) {
    return (move & FROM_MASK) >> FROM_SHIFT;
}

static inline int get_to_square(Move move) {
    return (move & TO_MASK) >> TO_SHIFT;
}

static inline int get_piece_index(Move move) {
    return (move & PIECE_INDEX_MASK) >> PIECE_INDEX_SHIFT;
}

static inline int get_move_type(Move move) {
    return (move & MOVE_TYPE_MASK) >> MOVE_TYPE_SHIFT;
}

static inline int get_capture_flag(Move move) {
    return (move & CAPTURE_MASK) >> CAPTURE_SHIFT;
}

static inline int get_promotion_type(Move move) {
    return (move & PROMOTION_TYPE_MASK) >> PROMOTION_TYPE_SHIFT;
}

static inline int get_captured_index(Move move) {
    return (move & CAPTURED_INDEX_MASK) >> CAPTURED_INDEX_SHIFT;
}




struct piece {
    int type;
    int colour;
    int rank;
    int file;
    int alive;
    int value;
};

struct Evaluation {
    uint8_t white_phase;
    uint8_t black_phase;
    
    int16_t mg_evaluation;
    int16_t eg_evaluation;

    uint16_t white_material;
    uint16_t black_material;

    uint8_t white_bishops;
    uint8_t black_bishops;

    uint32_t white_pawn_files;
    uint32_t black_pawn_files;
};

struct GameState {
    int board[8][8];
    int Index_board[8][8];
    int halfmove_clock;
    int ply;
    struct piece white_pieces[16];
    struct piece black_pieces[16];
    int side_to_move;
    int iteration;
    uint64_t zobrist_hash_3fold_history[512];
    uint64_t zobrist_hash;
    int game;
    uint8_t castling_rights;
    int en_passant_square;
    int white_king_index;
    int black_king_index;
    int white_king_square;
    int black_king_square;
    struct Evaluation eval;
    float time;
};

struct SearchParams {
    struct GameState Game;
    int depth;
    int time_left;
    int increment;
    int move_time;
};

struct MoveInfo {
    int moving_piece_index;
    int moving_piece_type;
    int captured_piece_index;

    int move_type;

    int start_square;
    int start_rank;
    int start_file;

    int target_square;
    int target_rank;
    int target_file;

    int moving_to_file;
    int moving_to_rank;

    int rook_to_file;

    int capture_flag;
    int promotion_type;
};


struct UndoInfo {
    uint8_t castling_rights;
    int white_king_square;
    int black_king_square;
    int halfmove_clock;
    int en_passant_square;
    int captured_piece_index;
    uint64_t zobrist_hash;
    struct Evaluation eval;
};


struct EngineStats {
    uint64_t nodes;
    int qnodes;
    int checkEvaluations;
    int abPrunes;
    int qcheckmate;
    int checkmate;
    int stalemate;
    int enpassant;
    int castle;
    int capture_cutoff;
    int quiet_cutoff;
    int standpat_cutoff;
    int killer_cutoff;
    int cutoffs;
    int cutoffmove[100];
    int qcutoffmove[100];
    int hash_move_cutoff;
    int hash_move;
};

struct RootMove {
    Move Move;
    int eval;
};


struct SearchContext {
    struct EngineStats es;

    Move pvTable[64][64];
    int pvLength[64];

    Move killer_moves[64][2];
    int history_table[64][64];

    int max_depth;
    struct timespec start, now;
    int timed_out;
    int time_limit_ms;
    uint64_t nodes;

};

struct NodeState {
    int ply;
    int depth;

    int alpha;
    int beta;

    Move last_played_move;
    int on_pv;

    int null_node;
};



#endif

