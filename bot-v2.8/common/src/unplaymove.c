
#include "structs.h"
#include <stdlib.h>





void unplay_move(Move played_move, struct GameState* gs, struct UndoInfo* ui){


    // Revert castling rights and ep square, and side to move
    (gs->ply)--;
    gs->castling_rights = ui->castling_rights;
    gs->en_passant_square = ui->en_passant_square;
    gs->white_king_square = ui->white_king_square;
    gs->black_king_square = ui->black_king_square;
    gs->halfmove_clock = ui->halfmove_clock;
    gs->side_to_move = !gs->side_to_move;
    gs->zobrist_hash = ui->zobrist_hash;
    int i    = ui->captured_piece_index;

    int pInd = get_piece_index(played_move);
    int start_file  = get_from_square(played_move) % 8;
    int start_rank  = get_from_square(played_move) / 8;
    int target_file  = get_to_square(played_move) % 8;
    int target_rank  = get_to_square(played_move) / 8;


    struct piece* aPieces = gs->side_to_move ? gs->white_pieces : gs->black_pieces;
    struct piece* ePieces = gs->side_to_move ? gs->black_pieces : gs->white_pieces;

    struct piece* moved_piece    = &aPieces[pInd];
    struct piece* captured_piece = &ePieces[i];
    struct piece* castled_piece  = &aPieces[i];

    int rank_capture = target_rank;

    // Switch case for handling normal and special moves
    switch (get_move_type(played_move)) {
        case EN_PASSANT:
            rank_capture = start_rank; // In en passant, the captured piece is on the starting rank
            break;

        case SHORT_CASTLE: //short and long castle
        case LONG_CASTLE:
            // clear rook spot
            gs->board[castled_piece->rank][castled_piece->file] = EMPTY;
            gs->Index_board[castled_piece->rank][castled_piece->file] = -1;

            // Put the rook back
            castled_piece->rank  = target_rank;
            castled_piece->file  = target_file;
            gs->board[target_rank][target_file] = castled_piece->type;
            gs->Index_board[target_rank][target_file] = i;
            break;

        case PROMOTION: // Promotion
            moved_piece->type  = gs->side_to_move ? PAWN : -PAWN;
            moved_piece->value = 1;
            break;
    }
    // Changing position of moved piece
    gs->board[moved_piece->rank][moved_piece->file] = EMPTY;
    gs->Index_board[moved_piece->rank][moved_piece->file] = -1;
    moved_piece->rank = start_rank;
    moved_piece->file = start_file;
    gs->board[start_rank][start_file] = moved_piece->type;
    gs->Index_board[start_rank][start_file] = pInd;


    // Alives captured piece
    if (get_capture_flag(played_move) == CAPTURE) {
            captured_piece->alive = 1;
            gs->board[rank_capture][target_file] = captured_piece->type;
            gs->Index_board[rank_capture][target_file] = i;
    }
}





   
