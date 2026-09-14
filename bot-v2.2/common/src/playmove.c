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


void play_move(uint32_t played_move, struct GameState* gs, struct UndoInfo* ui){



    // Record stateinfo
    ui->castling_rights = gs->castling_rights;
    ui->en_passant_square = gs->en_passant_square;
    ui->white_king_square = gs->white_king_square;
    ui->black_king_square = gs->black_king_square;
    ui->halfmove_clock = gs->halfmove_clock;
    ui->zobrist_hash_3fold = gs->zobrist_hash_3fold;
    uint64_t hash = ui->zobrist_hash_3fold;
    gs->zobrist_hash_3fold_history[gs->ply] = hash;

    // Initialize
    int pInd = get_piece_index(played_move);
    struct piece* aPieces = gs->side_to_move ? gs->white_pieces : gs->black_pieces;
    struct piece* ePieces = gs->side_to_move ? gs->black_pieces : gs->white_pieces;
    struct piece* moving_piece = &aPieces[pInd]; 
    int starting_piece_type   = moving_piece->type;

    // start and target squares
    int start_square = get_from_square(played_move);
    int start_rank = start_square >> 3;
    int start_file = start_square & 7;
    int target_square = get_to_square(played_move);
    int target_file  = target_square & 7;
    int target_rank  = target_square >> 3;
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
        int index = gs->Index_Board[rank_capture][target_file];
        // clear board and kill piece
        gs->Board[rank_capture][target_file] = EMPTY;
        gs->Index_Board[rank_capture][target_file] = -1;
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
    gs->Board[moving_to_rank][moving_to_file] = final_piece_type;
    gs->Board[start_rank][start_file] = EMPTY; 
    gs->Index_Board[moving_to_rank][moving_to_file] = pInd;
    gs->Index_Board[start_rank][start_file] = -1;

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
        int index = gs->Index_Board[target_rank][target_file];
        // Remove piece
        hash ^= zobrist_pieces[map(aPieces[index].type)][target_rank * 8 + target_file];
        gs->Board[target_rank][target_file] = EMPTY;
        gs->Index_Board[target_rank][target_file] = -1;

        // Add back
        aPieces[index].file = rook_to_file;

        hash ^= zobrist_pieces[map(aPieces[index].type)][target_rank * 8 + rook_to_file];
        gs->Board[start_rank][rook_to_file] = aPieces[index].type;
        ui->captured_piece_index = index;
        gs->Index_Board[start_rank][rook_to_file] = index;
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
    gs->zobrist_hash_3fold = hash;
    (gs->ply)++;

}

