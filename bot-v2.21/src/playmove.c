#include "types.h"
#include <stdlib.h>
#include <stdio.h>
#include "zobrist.h"
#include "pst.h"





static const uint8_t castling_mask[8][8] = {
    [0] = { ~(uint8_t)WQ, 0xF, 0xF, 0xF, ~(uint8_t)(WK | WQ), 0xF, 0xF, ~(uint8_t)WK },
    [1] = { 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF },
    [2] = { 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF },
    [3] = { 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF },
    [4] = { 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF },
    [5] = { 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF },
    [6] = { 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF },
    [7] = { ~(uint8_t)BQ, 0xF, 0xF, 0xF, ~(uint8_t)(BK | BQ), 0xF, 0xF, ~(uint8_t)BK }
};

static inline int map(int x) {
    return x > 0 ? x - 1 : 5 - x;
}

static inline void save_undo(struct GameState* Game, struct UndoInfo* Undo) {
    Undo->castling_rights = Game->castling_rights;
    Undo->en_passant_square = Game->en_passant_square;
    Undo->white_king_square = Game->white_king_square;
    Undo->black_king_square = Game->black_king_square;
    Undo->halfmove_clock = Game->halfmove_clock;
    Undo->zobrist_hash = Game->zobrist_hash;
}

static inline struct MoveInfo decode_move(Move played_move, struct GameState* Game) {

    struct MoveInfo Move = {0};

    // Initialize
    Move.moving_piece_index = get_piece_index(played_move);
    Move.move_type = get_move_type(played_move);
    Move.capture_flag = get_capture_flag(played_move);
    Move.promotion_type = get_promotion_type(played_move);

    // start and target squares
    Move.start_square = get_from_square(played_move);
    Move.start_rank = Move.start_square / 8;
    Move.start_file = Move.start_square % 8;
    Move.target_square = get_to_square(played_move);
    Move.target_file  = Move.target_square % 8;
    Move.target_rank  = Move.target_square / 8;


    Move.moving_piece_type = Game->board[Move.start_rank][Move.start_file];
    Move.moving_to_file = Move.target_file;
    Move.moving_to_rank = Move.target_rank;

    return Move;
}

static inline void update_castling_rights(struct GameState* Game, struct MoveInfo Move, uint64_t* hash) {

    // Remove old castling_rights
    uint8_t castling_rights = Game->castling_rights;
    *hash ^= zobrist_castling_rights[castling_rights];

    // Change castling rights
    castling_rights &= castling_mask[Move.start_rank][Move.start_file];
    castling_rights &= castling_mask[Move.target_rank][Move.target_file];  

    // Add back new
    *hash ^= zobrist_castling_rights[castling_rights];
    Game->castling_rights = castling_rights;
}

static inline void move_pieces(int start_rank, int start_file, int target_rank, int target_file, int original_type, struct piece* moving_piece, struct GameState* Game, uint64_t *hash) {

    moving_piece->rank = target_rank;
    moving_piece->file = target_file;

    int final_piece_type = moving_piece->type;
    int p_index = Game->Index_board[start_rank][start_file];

    // Update board and index board
    Game->board[target_rank][target_file] = final_piece_type;
    Game->board[start_rank][start_file] = EMPTY;
    Game->Index_board[target_rank][target_file] = p_index;
    Game->Index_board[start_rank][start_file] = -1;

    *hash ^= zobrist_pieces[map(original_type)][start_rank * 8 + start_file];
    *hash ^= zobrist_pieces[map(final_piece_type)][target_rank * 8 + target_file];
}


static inline void capture_piece(struct MoveInfo Move, struct GameState* Game, struct UndoInfo* Undo, struct piece ePieces[16], uint64_t *hash) {

    int rank_capture = (Move.move_type == EN_PASSANT) ? Move.start_rank : Move.target_rank;

    int index = Game->Index_board[rank_capture][Move.target_file];

    // clear board and kill piece
    Game->board[rank_capture][Move.target_file] = EMPTY;
    Game->Index_board[rank_capture][Move.target_file] = -1;
    *hash ^= zobrist_pieces[map(ePieces[index].type)][rank_capture * 8 + Move.target_file];
    ePieces[index].alive = 0;

    // record index of captured piece to help unplaymove
    Undo->captured_piece_index = index;
}

static inline void play_standard_move(struct MoveInfo Move, struct GameState* Game, struct UndoInfo* Undo, struct piece* moving_piece, struct piece ePieces[16], uint64_t *hash) {

    if (Move.capture_flag == CAPTURE) capture_piece(Move, Game, Undo, ePieces, hash);
    move_pieces(Move.start_rank, Move.start_file, Move.moving_to_rank, Move.moving_to_file, Move.moving_piece_type, moving_piece, Game, hash);
}

static inline void play_promotion_move(struct MoveInfo Move, struct GameState* Game, struct UndoInfo* Undo, struct piece* moving_piece, struct piece ePieces[16], uint64_t *hash) {
    
    static const int Promo[4]  = {QUEEN, ROOK, BISHOP, KNIGHT};
    static const int pieceValues[4] = {9, 5, 3, 3};

    // Sets the piece type of the pawn to whatever was input
    moving_piece->type = (Game->side_to_move == SIDE_WHITE) ? Promo[Move.promotion_type] : -Promo[Move.promotion_type];
    moving_piece->value = pieceValues[Move.promotion_type];

    play_standard_move(Move, Game, Undo, moving_piece, ePieces, hash);
}

static inline void play_castle_move(struct MoveInfo Move, struct GameState* Game, struct UndoInfo* Undo, struct piece* moving_piece, struct piece aPieces[16], uint64_t *hash) {

    Move.moving_to_file = (Move.move_type == SHORT_CASTLE) ? Move.target_file - 1 : Move.target_file + 2;
    Move.rook_to_file = (Move.move_type == SHORT_CASTLE) ? Move.start_file + 1 : Move.start_file - 1;

    int rook_index = Game->Index_board[Move.target_rank][Move.target_file];
    struct piece* rook = &aPieces[rook_index];

    Undo->captured_piece_index = rook_index;

    move_pieces(Move.target_rank, Move.target_file, Move.moving_to_rank, Move.rook_to_file, rook->type, rook, Game, hash);

    move_pieces(Move.start_rank, Move.start_file, Move.moving_to_rank, Move.moving_to_file, Move.moving_piece_type, moving_piece, Game, hash);
}

static inline void update_king_square(struct GameState* Game, struct piece* moving_piece) {
    int* king_square = (Game->side_to_move == SIDE_WHITE) ? &Game->white_king_square : &Game->black_king_square;
    *king_square = moving_piece->rank * 8 + moving_piece->file;
}

static inline void update_en_passant_square(struct GameState* Game, uint64_t *hash, struct MoveInfo Move) {
    // Remove old en passant square
    if (Game->en_passant_square != -1) {
        *hash ^= zobrist_en_passant_file[Game->en_passant_square & 7];
    }

    // Kill old en_passant_square and record new
    Game->en_passant_square = -1;
    if ((abs(Move.moving_piece_type) == PAWN) && abs(Move.target_rank - Move.start_rank) == 2)  {
        Game->en_passant_square = (Move.start_square + Move.target_square) / 2;
        *hash ^= zobrist_en_passant_file[Move.start_file];
    }
}

static inline void update_half_move_clock(struct MoveInfo Move, struct GameState* Game) {
    // Record and change halfmove_clock;
    if (Move.capture_flag == CAPTURE || abs(Move.moving_piece_type) == PAWN) {
        Game->halfmove_clock = 0;
    } else {
        Game->halfmove_clock++;
    }
}


static inline void update_evaluation(struct MoveInfo Move, struct GameState* Game, struct piece* moving_piece, struct piece aPieces[16], struct piece ePieces[16], struct UndoInfo* Undo) {
    int side_to_move = Game->side_to_move;

    int final_piece_type = abs(moving_piece->type);
    int final_rank       = moving_piece->rank;
    int final_file       = moving_piece->file;

    if (side_to_move == SIDE_WHITE) {
        int mg_eval_change = EarlyPST[final_piece_type][final_rank][final_file] - EarlyPST[Move.moving_piece_type][Move.start_rank][Move.start_file];
        int eg_eval_change = LatePST[final_piece_type][final_rank][final_file]  - LatePST[Move.moving_piece_type][Move.start_rank][Move.start_file];
        Game->eval.mg_evaluation += mg_eval_change;
        Game->eval.eg_evaluation += eg_eval_change;

        if (Move.moving_piece_type == PAWN) {
            Game->eval.white_pawns ^= (1ULL << ( 8 * Move.start_rank + Move.start_file));

            if (Move.move_type != PROMOTION) {
                Game->eval.white_pawns ^= (1ULL << ( 8 * final_rank + final_file));
            }
        }
    }
    else {
        int mg_eval_change = EarlyPST[final_piece_type][7 - final_rank][final_file] - EarlyPST[abs(Move.moving_piece_type)][7 - Move.start_rank][Move.start_file];
        int eg_eval_change = LatePST[final_piece_type][7 - final_rank][final_file]  - LatePST[abs(Move.moving_piece_type)][7 - Move.start_rank][Move.start_file];
        Game->eval.mg_evaluation -= mg_eval_change;
        Game->eval.eg_evaluation -= eg_eval_change;

        if (abs(Move.moving_piece_type) == PAWN) {
            Game->eval.black_pawns ^= (1ULL << ( 8 * Move.start_rank + Move.start_file));

            if (Move.move_type != PROMOTION) {
                Game->eval.black_pawns ^= (1ULL << ( 8 * final_rank + final_file));
            }
        }
    }

    if (Move.capture_flag == CAPTURE) {
        int type = abs(ePieces[Undo->captured_piece_index].type);
        int raw_rank = ePieces[Undo->captured_piece_index].rank;
        int file = ePieces[Undo->captured_piece_index].file;

        if (side_to_move == SIDE_WHITE) {
            int pst_rank = 7 - raw_rank;
            Game->eval.black_material -= piece_values[type];
            Game->eval.black_phase    -= game_stage_values[type];
            Game->eval.mg_evaluation  += EarlyPST[type][pst_rank][file];
            Game->eval.eg_evaluation  += LatePST[type][pst_rank][file];

            if (type == BISHOP) Game->eval.black_bishops--;

            if (type == PAWN) Game->eval.black_pawns ^=  (1ULL << (8 * raw_rank + file));
            
        } 
        else {
            int pst_rank = raw_rank;
            Game->eval.white_material -= piece_values[type];
            Game->eval.white_phase    -= game_stage_values[type];
            Game->eval.mg_evaluation  -= EarlyPST[type][pst_rank][file]; 
            Game->eval.eg_evaluation  -= LatePST[type][pst_rank][file];

            if (type == BISHOP) Game->eval.white_bishops--;
            if (type == PAWN) Game->eval.white_pawns ^=  (1ULL << (8 * raw_rank + file));
        }
    }

    if (Move.move_type == PROMOTION) {
        int promo_val = piece_values[final_piece_type] - piece_values[PAWN];
        int promo_phase = game_stage_values[final_piece_type];

        if (side_to_move == SIDE_WHITE) {
            Game->eval.white_material += promo_val;
            Game->eval.white_phase    += promo_phase;
            if (final_piece_type == BISHOP) Game->eval.white_bishops++;
        } 
        else {
            Game->eval.black_material += promo_val;
            Game->eval.black_phase    += promo_phase;
            if (final_piece_type == BISHOP) Game->eval.black_bishops++;
        }
    }

    if (Move.move_type == SHORT_CASTLE || Move.move_type == LONG_CASTLE) {
        int rook_file = aPieces[Undo->captured_piece_index].file;

        if (side_to_move == SIDE_WHITE) {
            int mg_eval_change = EarlyPST[ROOK][Move.target_rank][rook_file] - EarlyPST[ROOK][Move.target_rank][Move.target_file];
            int eg_eval_change = LatePST[ROOK][Move.target_rank][rook_file]  - LatePST[ROOK][Move.target_rank][Move.target_file];
            Game->eval.mg_evaluation += mg_eval_change;
            Game->eval.eg_evaluation += eg_eval_change;
        } 
        else {
            int rook_rank = 7 - Move.target_rank;
            int mg_eval_change = EarlyPST[ROOK][rook_rank][rook_file] - EarlyPST[ROOK][rook_rank][Move.target_file];
            int eg_eval_change = LatePST[ROOK][rook_rank][rook_file]  - LatePST[ROOK][rook_rank][Move.target_file];
            Game->eval.mg_evaluation -= mg_eval_change;
            Game->eval.eg_evaluation -= eg_eval_change;
        }
    }
}
void play_move(Move played_move, struct GameState* Game, struct UndoInfo* Undo){


    save_undo(Game, Undo);

    uint64_t hash = Undo->zobrist_hash;
    Game->zobrist_hash_3fold_history[Game->ply] = hash;

    struct MoveInfo Move = decode_move(played_move, Game);

    update_castling_rights(Game, Move, &hash);

    struct piece* aPieces = (Game->side_to_move == SIDE_WHITE) ? Game->white_pieces : Game->black_pieces;
    struct piece* ePieces = (Game->side_to_move == SIDE_WHITE) ? Game->black_pieces : Game->white_pieces;
    struct piece* moving_piece = &aPieces[Move.moving_piece_index]; 


    switch (Move.move_type) {
    case STANDARD:
        play_standard_move(Move, Game, Undo, moving_piece, ePieces, &hash);
        break;
    case PROMOTION:
        play_promotion_move(Move, Game, Undo, moving_piece, ePieces, &hash);
        break;
    case SHORT_CASTLE:
    case LONG_CASTLE:
        play_castle_move(Move, Game, Undo, moving_piece, aPieces, &hash);
        break;
    case EN_PASSANT:
        play_standard_move(Move, Game, Undo, moving_piece, ePieces, &hash);
        break;
    }


    if (abs(Move.moving_piece_type) == KING) {
        update_king_square(Game, moving_piece);
    }

    update_en_passant_square(Game, &hash, Move);

    update_half_move_clock(Move, Game);


    // End turn, store hash, and count ply
    Game->side_to_move = !Game->side_to_move;
    hash ^= zobrist_side_to_move;
    Game->zobrist_hash = hash;
    (Game->ply)++;

}
static inline void save_eval(struct GameState* Game, struct UndoInfo* Undo) {
    Undo->eval = Game->eval;
}
void make_move(Move played_move, struct GameState* Game, struct UndoInfo* Undo){


    save_undo(Game, Undo);
    save_eval(Game, Undo);

    uint64_t hash = Undo->zobrist_hash;
    Game->zobrist_hash_3fold_history[Game->ply] = hash;

    struct MoveInfo Move = decode_move(played_move, Game);

    update_castling_rights(Game, Move, &hash);

    struct piece* aPieces = (Game->side_to_move == SIDE_WHITE) ? Game->white_pieces : Game->black_pieces;
    struct piece* ePieces = (Game->side_to_move == SIDE_WHITE) ? Game->black_pieces : Game->white_pieces;
    struct piece* moving_piece = &aPieces[Move.moving_piece_index];


    switch (Move.move_type) {
    case STANDARD:
        play_standard_move(Move, Game, Undo, moving_piece, ePieces, &hash);
        break;
    case PROMOTION:
        play_promotion_move(Move, Game, Undo, moving_piece, ePieces, &hash);
        break;
    case SHORT_CASTLE:
    case LONG_CASTLE:
        play_castle_move(Move, Game, Undo, moving_piece, aPieces, &hash);
        break;
    case EN_PASSANT:
        play_standard_move(Move, Game, Undo, moving_piece, ePieces, &hash);
        break;
    }


    if (abs(Move.moving_piece_type) == KING) {
        update_king_square(Game, moving_piece);
    }

    update_en_passant_square(Game, &hash, Move);

    update_half_move_clock(Move, Game);

    update_evaluation(Move, Game, moving_piece, aPieces, ePieces, Undo);

    // End turn, store hash, and count ply
    Game->side_to_move = !Game->side_to_move;
    hash ^= zobrist_side_to_move;
    Game->zobrist_hash = hash;
    (Game->ply)++;

}


