#include "structs.h"
#include <stdlib.h>
#include <stdio.h>
#include "zobrist.h"





// Caslting mask used to deny caslting rights
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
// Helper function for pieces
static inline int map(int x) {
    return x > 0 ? x - 1 : 5 - x;
}

/*
static inline int register_move(int rank, int file, int pInd, int target_rank, int target_file, int capture, int type, int movetype) {

    int idx = *total_moves;

    Move move = ((rank * 8 + file) << FROM_SHIFT)
               | ((target_rank * 8 + target_file) << TO_SHIFT)
               | (pInd << PIECE_INDEX_SHIFT)
               | (movetype << MOVE_TYPE_SHIFT)
               | (capture << CAPTURE_SHIFT);

    if (movetype != PROMOTION) {
        legal_moves[idx] = move;
        *total_moves = idx + 1;
    }
    else {
        for (int i = 0; i < 4; i++) {
            legal_moves[idx + i] = move | (i << PROMOTION_TYPE_SHIFT);
        }
        *total_moves = idx + 4;
    }

}
*/

/*
int is_hash_move_legal(Move hash_move, struct GameState* gs) {

    // First verify the move information matches
    uint8_t from_square = get_from_square(hash_move);
    uint8_t to_square   = get_to_square(hash_move);
    uint8_t from_rank = from_square / 8;
    uint8_t from_file = from_square % 8;
    uint8_t to_rank   = to_square / 8;
    uint8_t to_file   = to_square % 8;

    uint8_t board_piece_index = gs->Index_board[from_rank][from_file];
    int piece_type = gs->board[from_rank][from_file];

    
    int allied_sign = (gs->side_to_move == SIDE_WHITE) ? 1 : -1;
    int aKing  = allied_sign * KING;
    int aQueen = allied_sign * QUEEN;
    int aRook  = allied_sign * ROOK;
    int aBish  = allied_sign * BISHOP;
    int aKni   = allied_sign * KNIGHT;
    int aPawn  = allied_sign * PAWN;

    // Assert its a firendly piece
    if (piece_type * allied_sign < 0) return 0;


    if (piece_type == aKni) {
        int dr = to_rank - from_rank;
        int df = to_file - from_file;
        if ((abs(dr) == 1 && abs(df) == 2)
                || ((abs(dr) == 2 && abs(df) == 1)) {
            if (gs->board[to_rank][to_file] == EMTPY) return register_move(from_rank, from_file, board_piece_index, to_rank, to_file, QUIET, piece_type, STANDARD) ;
            else if (gs->board[to_rank][to_file] * allied_sign < 0) return register_move(from_rank, from_file, board_piece_index, to_rank, to_file, CAPTURE, piece_type, STANDARD) ;
        }
        // NOT LEGAL
        return 0;
    }

    if (piece_type == aBish) { 
        int dr = to_rank - from_rank;
        int df = to_file - from_file;
        if (abs(dr) != abs(df)) return 0;
        int rank = from_rank + dr;
        int file = from_file + df;
        while (rank != ro_rank) {
            if (gs->board[rank][file] != EMTPY) return 0; 
            rank += dr;
            file += df;
        }
        if (gs->board[rank][file] == EMPTY) return register_move(from_rank, from_file, board_piece_index, to_rank, to_file, QUIET, piece_type, STANDARD) ;
        if (gs->board[rank][file] * allied_sign < 0) return register_move(from_rank, from_file, board_piece_index, to_rank, to_file, CAPTURE, piece_type, STANDARD) ;
        return 0;
    }


    if (piece_type == aRook) {
        int dr = to_rank - from_rank;
        int df = to_file - from_file;
        if (abs(dr)  1 && abs(df) == 0 || abs(dr)) return 0;
        int rank = from_rank + dr;
        int file = from_file + df;
        while (rank != ro_rank) {
            if (gs->board[rank][file] != EMTPY) return 0;
            rank += dr;
            file += df;
        }
        if (gs->board[rank][file] == EMPTY) return register_move(from_rank, from_file, board_piece_index, to_rank, to_file, QUIET, piece_type, STANDARD) ;
        if (gs->board[rank][file] * allied_sign < 0) return register_move(from_rank, from_file, board_piece_index, to_rank, to_file, CAPTURE, piece_type, STANDARD) ;

    }




    if (piece_type == aPawn) {
        int dr = to_rank - from_rank;
        int df = to_file - from_file;

        int starting_rank = (gs->side_to_move == SIDE_WHITE) ? 1 : 6;
        if (abs(dr) == 2){
            if (from_rank != starting_rank || df != 0) return 0;
            else if (get_capture_flag(hash_move) == QUIET && gs->board[to_rank][to_file] == EMPTY && gs->board[(to_rank + from_rank) /2][to_file] == EMPTY) return 1;
            return 0;

        if (abs(dr) == 1) {
            int pawn_direction = gs->side_to_move == SIDE_WHITE ? 1 : -1;
            if (dr == pawn_direction) {
                if (to_rank == 7 || to_rank == 0) {
                    int promotion_type = get_promotion_type(hash_move);
                    int move_type = get_move_type(hash_move);
                    if (promotion_type >= 4 || promotion_type < 0 || move_type != PROMOTION) return 0;
                }
                if (df == 0 && get_capture_flag(hash_move) == QUIET && gs->board[to_rank][to_file] == EMPTY) return 1;
                if ((gs->board[to_rank][to_file] * allied_sign < 0 || to_square == gs->en_passant_square) && get_capture_flag(hash_move) == CAPTURE) return 1;
            }
            return 0;
        }
        return 0;

    }

    uint8_t move_type = get_move_type(hash_move);

    switch (move_type) {
    case STANDARD:
        break;
    case EN_PASSANT:
        captured_rank = from_rank;
        int allied_pawn = (gs->side_to_move == SIDE_WHITE) 1 : -1;
        if (piece_type != allied_pawn) return 0;
        int enemy_pawn = (gs->side_to_move == SIDE_WHITE) -1 : 1;
        if (gs->board[capture_rank][to_file] != enemy_pawn) return 0;
        if (gs->en_passant_square != to_rank * 8 + to_file) return 0;
        break;
    case SHORT_CASTLE:
        if (gs->side_to_move == SIDE_WHITE) {
            if (!(gs->castling_rights & WK)) return 0;
        }
        else {
            if (!(gs->castling_rights & BK)) return 0;
        }
        int castle_rank = (gs->side_to_move == SIDE_WHITE) ? 0 : 7;
        if (castle_rank != from_rank) return 0;
        int allied_king = (gs->side_to_move == SIDE_WHITE) ? 6: -6;
        if (piece_type != allied_king) return 0;
        int rook_file = 7;
        int allied_rook = (gs->side_to_move == SIDE_WHITE) ? 5 : -5;
        if (gs->board[castle_rank][rook_file] != allied_rook) return 0; 

        int df = rook_file - from_file; 
        int file + df;
        while (file != rook_file) {
            if (gs->board[from_rank][file] != EMPTY) return 0; 
            file += df;
        }
        if (!is_king_safe(gs->board, from_rank, from_file, gs->side_to_move)
                || !is_king_safe(gs->board, from_rank, from_file + df, gs->side_to_move) 
                || !is_king_safe(gs->board, from_rank, from_file + 2*df, gs->side_to_move)) return 0;
        break;
    case LONG_CASTLE:
        if (gs->side_to_move == SIDE_WHITE) {
            if (!(gs->castling_rights & WQ)) return 0;
        }
        else {
            if (!(gs->castling_rights & BQ)) return 0;
        }
        int castle_rank = (gs->side_to_move == SIDE_WHITE) ? 0 : 7;
        if (castle_rank != from_rank) return 0;
        int allied_king = (gs->side_to_move == SIDE_WHITE) ? 6: -6;
        if (piece_type != allied_king) return 0;
        int rook_file = 0;
        int allied_rook = (gs->side_to_move == SIDE_WHITE) ? 5 : -5;
        if (gs->board[castle_rank][rook_file] != allied_rook) return 0;

        int df = rook_file - from_file;
        int file + df;
        while (file != rook_file) {
            if (gs->board[from_rank][file] != EMPTY) return 0;
            file += df;
        }
        if (!is_king_safe(gs->board, from_rank, from_file, gs->side_to_move)
                || !is_king_safe(gs->board, from_rank, from_file + df, gs->side_to_move)
                || !is_king_safe(gs->board, from_rank, from_file + 2*df, gs->side_to_move)) return 0;

        break;
    case PROMOTION:
        int promotion_piece = get_promotion_type(hash_move);
        if (promotion_piece >= 4 || promotion_piece < 0) return 0;
        int allied_pawn = (gs->side_to_move == SIDE_WHITE) 1 : -1;
        if (piece_type != allied_pawn) return 0;

        int promotion_rank = (gs->side_to_move == SIDE_WHITE) 7 : 0;
        if (to_rank != promotion_rank) return 0;
        if (from_rank != promotion_rank - allied_pawn) return 0;

    }
    
    if (get_capture_flag(hash_move) == CAPTURE) {
        int captured_piece = gs->board[captured_rank][to_file];
        if (!(captured_piece * piece_type < 0)) return 0;
    }


}
*/

void play_move(Move played_move, struct GameState* gs, struct UndoInfo* ui){



    // Record undoinfo
    ui->castling_rights = gs->castling_rights;
    ui->en_passant_square = gs->en_passant_square;
    ui->white_king_square = gs->white_king_square;
    ui->black_king_square = gs->black_king_square;
    ui->halfmove_clock = gs->halfmove_clock;
    ui->zobrist_hash = gs->zobrist_hash;
    uint64_t hash = ui->zobrist_hash;
    gs->zobrist_hash_3fold_history[gs->ply] = hash;

    // Initialize
    int pInd = get_piece_index(played_move);
    struct piece* aPieces = gs->side_to_move ? gs->white_pieces : gs->black_pieces;
    struct piece* ePieces = gs->side_to_move ? gs->black_pieces : gs->white_pieces;
    struct piece* moving_piece = &aPieces[pInd]; 
    int starting_piece_type   = moving_piece->type;

    // start and target squares
    int start_square = get_from_square(played_move);
    int start_rank = start_square / 8;
    int start_file = start_square % 8;
    int target_square = get_to_square(played_move);
    int target_file  = target_square % 8;
    int target_rank  = target_square / 8;
    int rank_capture = 0; // rank of captured piece (depends on movetype)


    int moving_to_file = target_file;
    int moving_to_rank = target_rank;

    int rook_to_file   = -1;


    // Remove old castling_rights
    uint8_t castling_rights = gs->castling_rights;
    hash ^= zobrist_castling_rights[castling_rights];

    // Change castling rights
    castling_rights &= castling_mask[start_rank][start_file];
    castling_rights &= castling_mask[target_rank][target_file];  

    // Add back new
    hash ^= zobrist_castling_rights[castling_rights];
    gs->castling_rights = castling_rights;


    // Switch case for diffrent move types
    switch (get_move_type(played_move)) {
    case STANDARD:
        rank_capture = target_rank; // In a standard capture, the piece is on the target square

        break;
    case EN_PASSANT:
        rank_capture = start_rank; // In en passant, the piece is on the starting Rank
        break;
    case SHORT_CASTLE:
        moving_to_file -=  1;  // IN short castle, the the king ends om the square before the rook
        rook_to_file = 1 + start_file;    // and the rook ends one before the king start file.
                                                       
        break;
    case LONG_CASTLE:
        moving_to_file += 2;  // In long castle, the king ends two squares infront of the rook
        rook_to_file = start_file - 1;    // and the rook ends one before the king
         
        break;
    case PROMOTION:
        rank_capture = target_rank; // Standard in promotion (not garanteed to be a capture)

        int Promo[4]  = {QUEEN, ROOK, BISHOP, KNIGHT};
        int pieceValues[4] = {9, 5, 3, 3};

        // Sets the piece type of the pawn to whatever was input
        if (gs->side_to_move) {
            moving_piece->type = Promo[get_promotion_type(played_move)];
        }
        else {
            moving_piece->type = -Promo[get_promotion_type(played_move)];
        }
        moving_piece->value = pieceValues[get_promotion_type(played_move)];
        break;
    }

    // Kills captured piece
    if (get_capture_flag(played_move) == CAPTURE) {
        int index = gs->Index_board[rank_capture][target_file];
        // clear board and kill piece
        gs->board[rank_capture][target_file] = EMPTY;
        gs->Index_board[rank_capture][target_file] = -1;
        hash ^= zobrist_pieces[map(ePieces[index].type)][rank_capture * 8 + target_file];
        ePieces[index].alive = 0;

        // record index of captured piece to help unplaymove
        ui->captured_piece_index = index;
    }

    // Changing position of moved piece struct
    moving_piece->rank = moving_to_rank;
    moving_piece->file = moving_to_file;

    // and on board
    int final_piece_type = moving_piece->type;
    gs->board[moving_to_rank][moving_to_file] = final_piece_type;
    gs->board[start_rank][start_file] = EMPTY; 
    gs->Index_board[moving_to_rank][moving_to_file] = pInd;
    gs->Index_board[start_rank][start_file] = -1;

    // and on zobrist
    hash ^= zobrist_pieces[map(starting_piece_type)][start_rank * 8 + start_file];
    hash ^= zobrist_pieces[map(final_piece_type)][moving_to_rank * 8 + moving_to_file];

    // update king square
    if (abs(moving_piece->type) == KING) {
        int* king_square = (gs->side_to_move == SIDE_WHITE) ? &gs->white_king_square : &gs->black_king_square;
        *king_square = moving_to_rank * 8 + moving_to_file;
    }

    // Move rook in the case of castling
    if (get_move_type(played_move) == SHORT_CASTLE || get_move_type(played_move) == LONG_CASTLE) {
        int index = gs->Index_board[target_rank][target_file];
        // Remove piece
        hash ^= zobrist_pieces[map(aPieces[index].type)][target_rank * 8 + target_file];
        gs->board[target_rank][target_file] = EMPTY;
        gs->Index_board[target_rank][target_file] = -1;

        // Add back
        aPieces[index].file = rook_to_file;

        hash ^= zobrist_pieces[map(aPieces[index].type)][target_rank * 8 + rook_to_file];
        gs->board[start_rank][rook_to_file] = aPieces[index].type;
        ui->captured_piece_index = index;
        gs->Index_board[start_rank][rook_to_file] = index;
    }

    // Remove old en passant square
    if (gs->en_passant_square != -1) {
        hash ^= zobrist_en_passant_file[gs->en_passant_square & 7];
    }

    // Kill old en_passant_square and record new
    gs->en_passant_square = -1;
    if ((abs(moving_piece->type) == PAWN) && abs(target_rank - start_rank) == 2)  {
        int dr = (gs->side_to_move == SIDE_WHITE) ? 1 : -1;
        gs->en_passant_square = 8*(start_rank + dr) + start_file;
        hash ^= zobrist_en_passant_file[start_file];
    }

    // Record and change halfmove_clock;
    if (get_capture_flag(played_move) || abs(moving_piece->type) == PAWN) {
        gs->halfmove_clock = 0;
    } else {
        gs->halfmove_clock++;
    }

    // And finally end turn by swapping colour
    gs->side_to_move = !gs->side_to_move;
    hash ^= zobrist_side_to_move;
    gs->zobrist_hash = hash;
    (gs->ply)++;

}

