
// 
#include "structs.h"
#include "movegen.h"
#include "board.h"
#include "playmove.h"
#include "unplaymove.h"
#include "uci.h"

// NULL
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>



/*

static inline void register_move(Move legal_moves[256], int* total_moves, int rank, int file, int pInd, int piece_side, int target_rank, int target_file, int capture, int type, int movetype) {


    int idx = *total_moves;
    if (movetype != PROMOTION) {
        Move* m = &legal_moves[idx];

        m->start_rank = rank;
        m->start_file = file;
        m->target_rank = target_rank;
        m->target_file = target_file;
        m->colour = piece_side;
        m->piece_type = type;
        m->piece_struct_index = pInd;
        m->captured_piece_index = -1; // Change later (in play_move) if a piece was captured
        m->move_type = movetype;
        m->capture = capture;
        m->promotion_type = -1;

        *total_moves = idx + 1;
    }
    else { // If promotion, record 4 moves with diffrent promotion pieces
        for (int i = 0; i < 4; i++) {
            Move* m = &legal_moves[idx + i];

            m->start_rank = rank;
            m->start_file = file;
            m->target_rank = target_rank;
            m->target_file = target_file;
            m->colour = piece_side;
            m->piece_type = type;
            m->piece_struct_index = pInd;
            m->captured_piece_index = -1; // Change later (in play_move) if a piece was captured
            m->move_type = movetype;
            m->capture = capture;
            m->promotion_type = i;
        }
        *total_moves = idx + 4;
    }
}
*/

static inline void register_move(Move legal_moves[256], int* total_moves, int rank, int file, int pInd, int piece_side, int target_rank, int target_file, int capture, int type, int movetype) {
    /*
     Updates the list of moves with a new struct.
     */

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




void find_semi_moves(int* total_semi_quiet, int* total_semi_captures, Move semi_quiet_moves[256], Move semi_capture_moves[256], struct GameState* gs) {

    /* finds semi legal moves with no consideration for pins or check*/

    int type;
    int rank;
    int file;
    int pInd;
    int target_square;
    int piece_side = gs->side_to_move;
    struct piece* Pieces = gs->side_to_move ? gs->white_pieces : gs->black_pieces;


    // Allied pieces representation
    int allied_sign = (piece_side == SIDE_WHITE) ? 1 : -1;
    int aKing  = allied_sign * KING;
    int aQueen = allied_sign * QUEEN;
    int aRook  = allied_sign * ROOK;
    int aBish  = allied_sign * BISHOP;
    int aKni   = allied_sign * KNIGHT;
    int aPawn  = allied_sign * PAWN;

    // Rook, bishop and queen direction pairs
    static const int rdirs[8] = {1, -1, 0, 0, 1, 1, -1, -1};
    static const int fdirs[8] = {0, 0, 1, -1, 1, -1, -1, 1};

    // Knight leap directions
    static const int krdirs[8] = { 2, 2, -2, -2, 1, -1, 1, -1};
    static const int kfdirs[8] = { 1, -1, 1, -1, 2, 2, -2, -2};

    // Pawn directions
    static const int pfdirs[3] = {1, 0, -1};

    // Rookfile when unmoved
    static const int rookFile[2] = {0, 7};
    
    // Loop over alive pieces
    for(int i = 15; i >= 0; i--) {  
        if (Pieces[i].alive) {
            type = Pieces[i].type;
            rank = Pieces[i].rank;
            file = Pieces[i].file;
            pInd = i;

            int target_rank = rank;
            int target_file = file;

            // Sliding pieces
            if (type == aQueen || type == aBish || type == aRook) {
                
                // Rooks use first 4, bishops last 4, and queen all directions
                int startDir = (type == aBish) ? 4 : 0;
                int endDir   = (type == aRook) ? 4 : 8; 
                
                for (int k = startDir; k < endDir; k++) {

                    int target_rank = rank + rdirs[k];
                    int target_file = file + fdirs[k];

                    // While on board
                    while (target_rank >= 0 && target_rank < 8 && target_file >= 0 && target_file < 8) {

                        // piece type on target_square
                        target_square = gs->board[target_rank][target_file];

                        // If free, record move
                        if (target_square == EMPTY) {
                            register_move(semi_quiet_moves, total_semi_quiet, rank, file, pInd, piece_side, target_rank, target_file, QUIET, type, STANDARD);
                        }
                        // else record capture if enemy, and break since we hit a piece
                        else {
                            if (allied_sign * target_square < 0) {
                                register_move(semi_capture_moves, total_semi_captures, rank, file, pInd, piece_side, target_rank, target_file, CAPTURE, type, STANDARD);
                            }
                            break; 
                        }

                        // Continue walking
                        target_rank += rdirs[k];
                        target_file += fdirs[k];

                    }
                }
                continue;
            }

            
            // Knight jump 
            if (type == aKni) {
                // Loop over all 8 possible jumps
                for (int k = 0; k < 8; k++) {

                    // Target square and type
                    target_rank = rank + krdirs[k];
                    target_file = file + kfdirs[k];

                    // If on board record moves
                    if (target_rank < 8 && target_rank >= 0 && target_file < 8 && target_file >= 0) {
                        target_square = gs->board[target_rank][target_file];

                        // If free, record quiet move
                        if (gs->board[target_rank][target_file] == EMPTY) {
                            register_move(semi_quiet_moves, total_semi_quiet, rank, file, pInd, piece_side, target_rank, target_file, QUIET, type, STANDARD);
                        }
                        // Else if enemy record capture
                        else if (target_square * allied_sign < 0) {
                            register_move(semi_capture_moves, total_semi_captures, rank, file, pInd, piece_side, target_rank, target_file, CAPTURE, type, STANDARD);
                        }
                    }
                        
                }
                continue;
            }


    
            if (type == aPawn) {
                for (int k = 0; k < 3; k++) {
                    // Pawn directions, rank always up/down
                    target_file = file + pfdirs[k];
                    target_rank = rank + allied_sign;

                    // If target rank is the last rank, set promotion movetype
                    int movetype = (target_rank == 7 || target_rank == 0) ? PROMOTION : STANDARD;

                    // Check if on board after one step
                    if (target_rank < 8 && target_rank >= 0 && target_file < 8 && target_file >= 0) {

                        // Type
                        target_square = gs->board[target_rank][target_file];


                        // If free and fdir = 0 -> record quiet walk and check if two step walk allowed
                        if (target_square == EMPTY && pfdirs[k] == 0) {
                            register_move(semi_quiet_moves, total_semi_quiet, rank, file, pInd, piece_side, target_rank, target_file, QUIET, type, movetype);

                            // Check if we are allowed one more step
                            if (((piece_side && (rank == 1)) || (!piece_side && rank == 6))) {
                                target_file = file;
                                target_rank = rank + 2 * allied_sign;

                                // Always on gs->board since df = 0 and we are on starting rank
                                target_square = gs->board[target_rank][target_file];
                                if (target_square == EMPTY) {
                                    register_move(semi_quiet_moves, total_semi_quiet, rank, file, pInd, piece_side, target_rank, target_file, QUIET, type, STANDARD);
                                }
                            }
                        }
                        // Possible capture? 
                        else if (target_square * allied_sign < 0 && pfdirs[k] != 0) {
                            register_move(semi_capture_moves, total_semi_captures, rank, file, pInd, piece_side, target_rank, target_file, CAPTURE, type, movetype);
                        }
                        
                        // En passant check
                        else if (gs->en_passant_square != -1 
                                && target_rank == gs->en_passant_square/8 
                                && target_file == gs->en_passant_square % 8) {
                            register_move(semi_capture_moves, total_semi_captures, rank, file, pInd, piece_side, target_rank, target_file, CAPTURE, type, EN_PASSANT);
                        }
                    }
                }
            }


            // King
            if (type == aKing) {
                
                for (int k = 0; k < 8; k++) {
                    // Same directions as sliding pieces, but only 1 step
                    target_rank = rank + rdirs[k];
                    target_file = file + fdirs[k];
                    // If on board
                    if (target_rank < 8 && target_rank >= 0 && target_file < 8 && target_file >= 0) {
                        target_square = gs->board[target_rank][target_file];

                        // If free, record quiet move
                        if (target_square == EMPTY) {
                            register_move(semi_quiet_moves, total_semi_quiet, rank, file, pInd, piece_side, target_rank, target_file, QUIET, type, STANDARD);
                        }
                        // Else if enemy record capture
                        else if (target_square * allied_sign < 0) {
                            register_move(semi_capture_moves, total_semi_captures, rank, file, pInd, piece_side, target_rank, target_file, CAPTURE, type, STANDARD);
                        }
                    }

                }

                // Castle check using bitwise castling rights

                // Look at only the rookfiles of alive and unmoved rooks
                uint8_t castling_rights = gs->castling_rights;
                int short_castle = (gs->side_to_move == SIDE_WHITE) ? WK : BK;
                int long_castle = (gs->side_to_move == SIDE_WHITE) ? WQ : BQ;
                int startDf = ((castling_rights & long_castle)) ? 0 : 1;
                int endDf = ((castling_rights & short_castle)) ? 2 : 1;

                for (int k = startDf; k < endDf; k++) {

                    int df = file > rookFile[k] ?   -1 :   1; // short and long castle direction
                    int movetype = df < 0 ? LONG_CASTLE : SHORT_CASTLE; // Short and long castle distingtion for play_move

                    // Verify the path is clear
                    int blocked = 0;
                    for (int path_file = file + df; path_file != rookFile[k]; path_file = path_file + df) {
                        if (gs->board[rank][path_file] != EMPTY) {
                            blocked = 1;
                        }
                    }

                    // Verfiy the path is safe
                    if (!blocked && is_king_safe(gs->board, rank, file, piece_side)
                                 && is_king_safe(gs->board, rank, file + df, piece_side)
                                 && is_king_safe(gs->board, rank, file + 2*df, piece_side)) {
                        register_move(semi_quiet_moves, total_semi_quiet, rank, file, pInd, piece_side, rank, rookFile[k], QUIET, type, movetype);
                    }
                }
            }
        }
    }
}



static inline int find_pinned_and_checking_pieces(const struct GameState* gs, int pinned_pieces_square[16], int pinned_rdir[16], int pinned_fdir[16], int checking_pieces_square[16], int* total_pinned_pieces, int* total_checking_pieces, int board[8][8]) {

    /*
     Finds pinned pieces, their square and their direction.
     Finds checking pieces and their square
     */


    int king_square = (gs->side_to_move == SIDE_WHITE) ? gs->white_king_square : gs->black_king_square;
    int rank = king_square / 8;
    int file = king_square % 8;
 

    int enemy_sign = (gs->side_to_move == SIDE_WHITE) ? -1 : 1;
    int ePawn  = enemy_sign * PAWN;
    int eKni   = enemy_sign * KNIGHT;
    int eBish  = enemy_sign * BISHOP;
    int eRook  = enemy_sign * ROOK;
    int eQueen = enemy_sign * QUEEN;

    // Straight and diagonal lines
    static const int rdirs[8] = {1, -1, 0, 0, 1, 1, -1, -1};
    static const int fdirs[8] = {0, 0, 1, -1, 1, -1, -1, 1};

    for (int k = 0; k < 8; k++) {

        // Step in each direction
        int pinned_piece_rank = -1;
        int pinned_piece_file = -1;
        
        int target_rank = rank + rdirs[k];
        int target_file = file + fdirs[k];

        while (target_rank >= 0 && target_rank < 8 && target_file >= 0 && target_file < 8) {

            int piece = board[target_rank][target_file];

            // If piece
            if (piece != 0) {

                // Straight lines
                if (k < 4) {
                    if (piece == eRook || piece == eQueen) {
                        if (pinned_piece_rank != -1) {
                            pinned_pieces_square[*total_pinned_pieces] = 8* pinned_piece_rank + pinned_piece_file;
                            pinned_rdir[*total_pinned_pieces] = rdirs[k];
                            pinned_fdir[*total_pinned_pieces] = fdirs[k];
                            (*total_pinned_pieces)++;
                            break;
                        }
                        else { 
                            checking_pieces_square[*total_checking_pieces] = 8 * target_rank + target_file;
                            (*total_checking_pieces)++;
                            break;
                        }
                    }

                }
                // Diagonal lines
                else {
                    if (piece == eBish || piece == eQueen) {
                        if (pinned_piece_rank != -1) { 
                            pinned_pieces_square[*total_pinned_pieces] = 8* pinned_piece_rank + pinned_piece_file;
                            pinned_rdir[*total_pinned_pieces] = rdirs[k];
                            pinned_fdir[*total_pinned_pieces] = fdirs[k];
                            (*total_pinned_pieces)++;
                            break;
                        }
                        else {
                            checking_pieces_square[*total_checking_pieces] = 8 * target_rank + target_file;
                            (*total_checking_pieces)++;
                            break;
                        }
                    }
                }
                
                // If not enemy:     
                if (pinned_piece_rank != -1) break; // Break if second pinned piece found
                if (piece * enemy_sign > 0) break; // BReak if non checking enemy

                // Or record the first potential pinned piece
                pinned_piece_rank = target_rank;
                pinned_piece_file = target_file;
            }

            target_rank += rdirs[k];
            target_file += fdirs[k];
        }
    }

    // Knight jumps
    static const int krdirs[8] = {2, 2, -2, -2, 1, -1, 1, -1};
    static const int kfdirs[8] = {1, -1, 1, -1, 2, 2, -2, -2};

    for (int k = 0; k < 8; k++) {
        int target_rank = rank + krdirs[k];
        int target_file = file + kfdirs[k];
        if (target_rank >= 0 && target_rank < 8 && target_file >= 0 && target_file < 8) {
            if (board[target_rank][target_file] == eKni) {
                checking_pieces_square[*total_checking_pieces] = 8 * target_rank + target_file;
                (*total_checking_pieces)++;
            }
        }
    }


    // Check diagonals infrom of king for pawns
    int pawn_dir = (gs->side_to_move) ? 1 : -1;
    int target_rank = rank + pawn_dir;

    if (target_rank >= 0 && target_rank < 8) {
        if (file - 1 >= 0 && board[target_rank][file - 1] == ePawn) {
            checking_pieces_square[*total_checking_pieces] = 8 * target_rank + file - 1;
            (*total_checking_pieces)++;
        }
        if (file + 1 < 8 && board[target_rank][file + 1] == ePawn) { 
            checking_pieces_square[*total_checking_pieces] = 8 * target_rank + file + 1;
            (*total_checking_pieces)++;
        }
    }
    return 1; // REturn does nothing
}

static inline void generate_pseudo_captures(int* total_semi_captures, Move semi_capture_moves[256], struct GameState* gs, int pinned_pieces_squares[16], int total_pinned_pieces, int pinned_rdirs[16], int pinned_fdirs[16]) {

    /* 
     Finds psuedo legal captures taking into account pins
     Only ever used when NOT in check
     */

    int type;
    int rank;
    int file;
    int pInd;
    int target_square;
    int piece_side = gs->side_to_move;
    struct piece* Pieces = gs->side_to_move ? gs->white_pieces : gs->black_pieces;


    // Allied pieces representation
    int allied_sign = (piece_side == SIDE_WHITE) ? 1 : -1;
    int aKing  = allied_sign * KING;
    int aQueen = allied_sign * QUEEN;
    int aRook  = allied_sign * ROOK;
    int aBish  = allied_sign * BISHOP;
    int aKni   = allied_sign * KNIGHT;
    int aPawn  = allied_sign * PAWN;

    // Rook, bishop and queen direction pairs
    static const int rdirs[8] = {1, -1, 0, 0, 1, 1, -1, -1};
    static const int fdirs[8] = {0, 0, 1, -1, 1, -1, -1, 1};

    // Knight leap directions
    static const int krdirs[8] = { 2, 2, -2, -2, 1, -1, 1, -1};
    static const int kfdirs[8] = { 1, -1, 1, -1, 2, 2, -2, -2};

    // Pawn directions
    static const int pfdirs[3] = {1, 0, -1};


    // Loop over alive pieces
    for(int i = 15; i >= 0; i--) {
        if (Pieces[i].alive) {
            type = Pieces[i].type;
            rank = Pieces[i].rank;
            file = Pieces[i].file;
            pInd = i;

            int target_rank = rank;
            int target_file = file;

            // If pinned, only allow moves that dont break it
            int pinned = 0;
            int pinned_piece = 0;
            for (int k = 0; k < total_pinned_pieces; k++) {
                if (rank * 8 + file == pinned_pieces_squares[k]) {
                    pinned = 1;
                    pinned_piece = k;
                }
            }


            // Sliding pieces
            if (type == aQueen || type == aBish || type == aRook) {

                // Rooks use first 4, bishops last 4, and queen all directions
                int startDir = (type == aBish) ? 4 : 0;
                int endDir   = (type == aRook) ? 4 : 8;

                for (int k = startDir; k < endDir; k++) {

                    // If pinned, only continue if the movemnet is on the same line/diagonal as pin
                    if (pinned) {
                        if ((rdirs[k] != pinned_rdirs[pinned_piece] || fdirs[k] != pinned_fdirs[pinned_piece])
                                && (-rdirs[k] != pinned_rdirs[pinned_piece] || -fdirs[k] != pinned_fdirs[pinned_piece])) {
                            continue;
                        }
                    }

                    int target_rank = rank + rdirs[k];
                    int target_file = file + fdirs[k];

                    // While on board
                    while (target_rank >= 0 && target_rank < 8 && target_file >= 0 && target_file < 8) {

                        // piece type on target_square
                        target_square = gs->board[target_rank][target_file];

                        // else record capture if enemy, and break since we hit a piece
                        if (target_square != EMPTY) {
                            if (allied_sign * target_square < 0) {
                                register_move(semi_capture_moves, total_semi_captures, rank, file, pInd, piece_side, target_rank, target_file, CAPTURE, type, STANDARD);
                            }
                            break;
                        }

                        // Continue walking
                        target_rank += rdirs[k];
                        target_file += fdirs[k];

                    }
                }
                continue;
            }


            // Knight jump
            if (type == aKni) {
                // If pinned, dont allow any moves
                if (pinned) {
                    continue;
                }
                // Loop over all 8 possible jumps
                for (int k = 0; k < 8; k++) {

                    // Target square and type
                    target_rank = rank + krdirs[k];
                    target_file = file + kfdirs[k];

                    // If on board record moves
                    if (target_rank < 8 && target_rank >= 0 && target_file < 8 && target_file >= 0) {
                        target_square = gs->board[target_rank][target_file];

                        // If free, record quiet move
                        if (gs->board[target_rank][target_file] != EMPTY) {
                        // Else if enemy record capture
                            if (target_square * allied_sign < 0) {
                                 register_move(semi_capture_moves, total_semi_captures, rank, file, pInd, piece_side, target_rank, target_file, CAPTURE, type, STANDARD);
                            }
                        }
                    }

                }
                continue;
            }



            if (type == aPawn) {
                for (int k = 0; k < 3; k++) {

                    if (pinned) {
                        int move_rdir = allied_sign;
                        int move_fdir = pfdirs[k];
                        int same_dir = (allied_sign != pinned_rdirs[pinned_piece] || move_fdir != pinned_fdirs[pinned_piece]);
                        int opp_dir  = (move_rdir != -pinned_rdirs[pinned_piece] || move_fdir != -pinned_fdirs[pinned_piece]);
                        if (same_dir && opp_dir) {
                            continue;
                        }
                    }
                    // Pawn directions, rank always up/down
                    target_file = file + pfdirs[k];
                    target_rank = rank + allied_sign;

                    // If target rank is the last rank, set promotion movetype
                    int movetype = (target_rank == 7 || target_rank == 0) ? PROMOTION : STANDARD;

                    // Check if on board after one step
                    if (target_rank < 8 && target_rank >= 0 && target_file < 8 && target_file >= 0) {

                        // Type
                        target_square = gs->board[target_rank][target_file];


                        // If free and fdir = 0 -> record quiet walk and check if two step walk allowed
                        if (target_square == EMPTY && pfdirs[k] == 0) {
                        }
                        // Possible capture?
                        else if (target_square * allied_sign < 0 && pfdirs[k] != 0) {
                            register_move(semi_capture_moves, total_semi_captures, rank, file, pInd, piece_side, target_rank, target_file, CAPTURE, type, movetype);
                        }

                        // En passant check
                        else if (gs->en_passant_square != -1
                                && target_rank == gs->en_passant_square/8
                                && target_file == gs->en_passant_square % 8) {
                            register_move(semi_capture_moves, total_semi_captures, rank, file, pInd, piece_side, target_rank, target_file, CAPTURE, type, EN_PASSANT);
                        }
                    }
                }
            }


            // King
            if (type == aKing) {

                for (int k = 0; k < 8; k++) {
                    // Same directions as sliding pieces, but only 1 step
                    target_rank = rank + rdirs[k];
                    target_file = file + fdirs[k];
                    // If on board
                    if (target_rank < 8 && target_rank >= 0 && target_file < 8 && target_file >= 0) {
                        target_square = gs->board[target_rank][target_file];

                        // If free, record quiet move
                        // Else if enemy record capture
                        if (target_square != EMPTY) {
                            if (target_square * allied_sign < 0) {
                                register_move(semi_capture_moves, total_semi_captures, rank, file, pInd, piece_side, target_rank, target_file, CAPTURE, type, STANDARD);
                            }
                        }
                    }

                }

            }
        }
    }
}



static inline void generate_pseudo_moves(int* total_semi_quiet, int* total_semi_captures, Move semi_quiet_moves[256], Move semi_capture_moves[256], struct GameState* gs, int pinned_pieces_squares[16], int total_pinned_pieces, int pinned_rdirs[16], int pinned_fdirs[16]) {

    /*
     Finds psuedo legal moves taking into consideration pins
     */

    int type;
    int rank;
    int file;
    int pInd;
    int target_square;
    int piece_side = gs->side_to_move;
    struct piece* Pieces = gs->side_to_move ? gs->white_pieces : gs->black_pieces;


    // Allied pieces representation
    int allied_sign = (piece_side == SIDE_WHITE) ? 1 : -1;
    int aKing  = allied_sign * KING;
    int aQueen = allied_sign * QUEEN;
    int aRook  = allied_sign * ROOK;
    int aBish  = allied_sign * BISHOP;
    int aKni   = allied_sign * KNIGHT;
    int aPawn  = allied_sign * PAWN;

    // Rook, bishop and queen direction pairs
    static const int rdirs[8] = {1, -1, 0, 0, 1, 1, -1, -1};
    static const int fdirs[8] = {0, 0, 1, -1, 1, -1, -1, 1};

    // Knight leap directions
    static const int krdirs[8] = { 2, 2, -2, -2, 1, -1, 1, -1};
    static const int kfdirs[8] = { 1, -1, 1, -1, 2, 2, -2, -2};

    // Pawn directions
    static const int pfdirs[3] = {1, 0, -1};

    // Rookfile when unmoved
    static const int rookFile[2] = {0, 7};

    // Loop over alive pieces
    for(int i = 15; i >= 0; i--) {
        if (Pieces[i].alive) {
            type = Pieces[i].type;
            rank = Pieces[i].rank;
            file = Pieces[i].file;
            pInd = i;

            int target_rank = rank;
            int target_file = file;
            
            // If pinned, mark it and record pinned_pieces_squares k 
            int pinned = 0;
            int pinned_piece = 0;
            for (int k = 0; k < total_pinned_pieces; k++) {
                if (rank * 8 + file == pinned_pieces_squares[k]) { 
                    pinned = 1;
                    pinned_piece = k;
                }
            }


            // Sliding pieces
            if (type == aQueen || type == aBish || type == aRook) {

                // Rooks use first 4, bishops last 4, and queen all directions
                int startDir = (type == aBish) ? 4 : 0;
                int endDir   = (type == aRook) ? 4 : 8;

                for (int k = startDir; k < endDir; k++) {

                    // If pinned, only continue if the movemnet is on the same line/diagonal as pin
                    if (pinned) {
                        if ((rdirs[k] != pinned_rdirs[pinned_piece] || fdirs[k] != pinned_fdirs[pinned_piece]) 
                                && (-rdirs[k] != pinned_rdirs[pinned_piece] || -fdirs[k] != pinned_fdirs[pinned_piece])) {
                            continue;
                        }
                    }

                    int target_rank = rank + rdirs[k];
                    int target_file = file + fdirs[k];

                    // While on board
                    while (target_rank >= 0 && target_rank < 8 && target_file >= 0 && target_file < 8) {

                        // piece type on target_square
                        target_square = gs->board[target_rank][target_file];

                        // If free, record move
                        if (target_square == EMPTY) {
                            register_move(semi_quiet_moves, total_semi_quiet, rank, file, pInd, piece_side, target_rank, target_file, QUIET, type, STANDARD);
                        }
                        // else record capture if enemy, and break since we hit a piece
                        else {
                            if (allied_sign * target_square < 0) {
                                register_move(semi_capture_moves, total_semi_captures, rank, file, pInd, piece_side, target_rank, target_file, CAPTURE, type, STANDARD);
                            }
                            break;
                        }

                        // Continue walking
                        target_rank += rdirs[k];
                        target_file += fdirs[k];

                    }
                }
                continue;
            }


            // Knight jump
            if (type == aKni) {
                // If pinned, dont allow any moves
                if (pinned) {
                    continue;
                }
                // Loop over all 8 possible jumps
                for (int k = 0; k < 8; k++) {

                    // Target square and type
                    target_rank = rank + krdirs[k];
                    target_file = file + kfdirs[k];

                    // If on board record moves
                    if (target_rank < 8 && target_rank >= 0 && target_file < 8 && target_file >= 0) {
                        target_square = gs->board[target_rank][target_file];

                        // If free, record quiet move
                        if (gs->board[target_rank][target_file] == EMPTY) {
                            register_move(semi_quiet_moves, total_semi_quiet, rank, file, pInd, piece_side, target_rank, target_file, QUIET, type, STANDARD);
                        }
                        // Else if enemy record capture
                        else if (target_square * allied_sign < 0) {
                            register_move(semi_capture_moves, total_semi_captures, rank, file, pInd, piece_side, target_rank, target_file, CAPTURE, type, STANDARD);
                        }
                    }

                }
                continue;
            }



            if (type == aPawn) {
                for (int k = 0; k < 3; k++) {

                    if (pinned) {
                        int move_rdir = allied_sign;
                        int move_fdir = pfdirs[k];
                        int same_dir = (allied_sign != pinned_rdirs[pinned_piece] || move_fdir != pinned_fdirs[pinned_piece]);
                        int opp_dir  = (move_rdir != -pinned_rdirs[pinned_piece] || move_fdir != -pinned_fdirs[pinned_piece]);
                        if (same_dir && opp_dir) {
                            continue;
                        }
                    }
                    // Pawn directions, rank always up/down
                    target_file = file + pfdirs[k];
                    target_rank = rank + allied_sign;

                    // If target rank is the last rank, set promotion movetype
                    int movetype = (target_rank == 7 || target_rank == 0) ? PROMOTION : STANDARD;

                    // Check if on board after one step
                    if (target_rank < 8 && target_rank >= 0 && target_file < 8 && target_file >= 0) {

                        // Type
                        target_square = gs->board[target_rank][target_file];


                        // If free and fdir = 0 -> record quiet walk and check if two step walk allowed
                        if (target_square == EMPTY && pfdirs[k] == 0) {
                            register_move(semi_quiet_moves, total_semi_quiet, rank, file, pInd, piece_side, target_rank, target_file, QUIET, type, movetype);

                            // Check if we are allowed one more step
                            if (((piece_side && (rank == 1)) || (!piece_side && rank == 6))) {
                                target_file = file;
                                target_rank = rank + 2 * allied_sign;

                                // Always on gs->board since df = 0 and we are on starting rank
                                target_square = gs->board[target_rank][target_file];
                                if (target_square == EMPTY) {
                                    register_move(semi_quiet_moves, total_semi_quiet, rank, file, pInd, piece_side, target_rank, target_file, QUIET, type, STANDARD);
                                }
                            }
                        }
                        // Possible capture?
                        else if (target_square * allied_sign < 0 && pfdirs[k] != 0) {
                            register_move(semi_capture_moves, total_semi_captures, rank, file, pInd, piece_side, target_rank, target_file, CAPTURE, type, movetype);
                        }

                        // En passant check
                        else if (gs->en_passant_square != -1
                                && target_rank == gs->en_passant_square/8
                                && target_file == gs->en_passant_square % 8) {
                            register_move(semi_capture_moves, total_semi_captures, rank, file, pInd, piece_side, target_rank, target_file, CAPTURE, type, EN_PASSANT);
                        }
                    }
                }
            }


            // King
            if (type == aKing) {

                for (int k = 0; k < 8; k++) {
                    // Same directions as sliding pieces, but only 1 step
                    target_rank = rank + rdirs[k];
                    target_file = file + fdirs[k];
                    // If on board
                    if (target_rank < 8 && target_rank >= 0 && target_file < 8 && target_file >= 0) {
                        target_square = gs->board[target_rank][target_file];

                        // If free, record quiet move
                        if (target_square == EMPTY) {
                            register_move(semi_quiet_moves, total_semi_quiet, rank, file, pInd, piece_side, target_rank, target_file, QUIET, type, STANDARD);
                        }
                        // Else if enemy record capture
                        else if (target_square * allied_sign < 0) {
                            register_move(semi_capture_moves, total_semi_captures, rank, file, pInd, piece_side, target_rank, target_file, CAPTURE, type, STANDARD);
                        }
                    }

                }

                // Castle check using bitwise castling rights

                // Look at only the rookfiles of alive and unmoved rooks
                uint8_t castling_rights = gs->castling_rights;
                int short_castle = (gs->side_to_move == SIDE_WHITE) ? WK : BK;
                int long_castle = (gs->side_to_move == SIDE_WHITE) ? WQ : BQ;
                int startDf = ((castling_rights & long_castle)) ? 0 : 1;
                int endDf = ((castling_rights & short_castle)) ? 2 : 1;

                for (int k = startDf; k < endDf; k++) {

                    int df = file > rookFile[k] ?   -1 :   1; // short and long castle direction
                    int movetype = df < 0 ? LONG_CASTLE : SHORT_CASTLE; // Short and long castle distingtion for play_move

                    // Verify the path is clear
                    int blocked = 0;
                    for (int path_file = file + df; path_file != rookFile[k]; path_file = path_file + df) {
                        if (gs->board[rank][path_file] != EMPTY) {
                            blocked = 1;
                        }
                    }

                    // Verfiy the path is safe
                    if (!blocked && is_king_safe(gs->board, rank, file, piece_side)
                                 && is_king_safe(gs->board, rank, file + df, piece_side)
                                 && is_king_safe(gs->board, rank, file + 2*df, piece_side)) {
                        register_move(semi_quiet_moves, total_semi_quiet, rank, file, pInd, piece_side, rank, rookFile[k], QUIET, type, movetype);
                    }
                }
            }
        }
    }
}


static inline void generate_pseudo_evasions(int* total_semi_quiet, int* total_semi_captures, Move semi_quiet_moves[256], Move semi_capture_moves[256], struct GameState* gs, int pinned_pieces_squares[16], int total_pinned_pieces, int pinned_rdirs[16], int pinned_fdirs[16], int checking_pieces_squares[16], int total_checking_pieces) {

    /* Finds psuedo legal moves evading check*/

    int type;
    int rank;
    int file;
    int pInd;
    int target_square;
    int piece_side = gs->side_to_move;
    struct piece* Pieces = gs->side_to_move ? gs->white_pieces : gs->black_pieces;


    // Allied pieces representation
    int allied_sign = (piece_side == SIDE_WHITE) ? 1 : -1;
    int aKing  = allied_sign * KING;
    int aQueen = allied_sign * QUEEN;
    int aRook  = allied_sign * ROOK;
    int aBish  = allied_sign * BISHOP;
    int aKni   = allied_sign * KNIGHT;
    int aPawn  = allied_sign * PAWN;

    // Rook, bishop and queen direction pairs
    static const int rdirs[8] = {1, -1, 0, 0, 1, 1, -1, -1};
    static const int fdirs[8] = {0, 0, 1, -1, 1, -1, -1, 1};

    // Knight leap directions
    static const int krdirs[8] = { 2, 2, -2, -2, 1, -1, 1, -1};
    static const int kfdirs[8] = { 1, -1, 1, -1, 2, 2, -2, -2};

    // Pawn directions
    static const int pfdirs[3] = {1, 0, -1};


    // If in double check, only allow king moves
    int king_only = 0;
    if (total_checking_pieces > 1) {
        king_only = 1;
    }

    // If in check, find allowed pieces squares to land on, blocking or capturing the checking piece
    int allowed_squares[8][8] = {0};
    if (total_checking_pieces == 1) {
        int king_square = (gs->side_to_move == SIDE_WHITE) ? gs->white_king_square : gs->black_king_square;
        int king_rank = king_square / 8;
        int king_file = king_square % 8;
    
        int checking_piece_rank = checking_pieces_squares[0] / 8;
        int checking_piece_file = checking_pieces_squares[0] % 8;

        allowed_squares[checking_piece_rank][checking_piece_file] = 1;
        
        int dr = checking_piece_rank - king_rank;
        int df = checking_piece_file - king_file;
        
        // Vertical lines
        if (dr == 0 && df != 0) {
            int target_file = king_file + df/abs(df);
            int target_rank = king_rank;
             
            while (target_file != checking_piece_file) {
                allowed_squares[target_rank][target_file] = 1;
                target_file += df/abs(df);
            }
        }
        // Horizontal lines
        if (dr != 0 && df == 0) {
            int target_rank= king_rank + dr/abs(dr);
            int target_file = king_file;

            while (target_rank != checking_piece_rank) {
                allowed_squares[target_rank][target_file] = 1;
                target_rank += dr/abs(dr);
            }
        }

        // Diagonals
        if (dr != 0 && df != 0 && (abs(dr) == abs(df))) {
            int target_rank= king_rank + dr/abs(dr);
            int target_file = king_file + df/abs(df);

            while (target_rank != checking_piece_rank) {
                allowed_squares[target_rank][target_file] = 1;
                target_rank += dr/abs(dr);
                target_file += df/abs(df);
            }
        }
    }

    // Loop over alive pieces
    for(int i = 15; i >= 0; i--) {
        if (Pieces[i].alive) {
            type = Pieces[i].type;
            rank = Pieces[i].rank;
            file = Pieces[i].file;
            pInd = i;

            int target_rank = rank;
            int target_file = file;

            int pinned = 0;
            int pinned_piece = 0;
            for (int k = 0; k < total_pinned_pieces; k++) {
                if (rank * 8 + file == pinned_pieces_squares[k]) {
                    pinned = 1;
                    pinned_piece = k;
                }
            }

            // Sliding pieces
            if ((type == aQueen || type == aBish || type == aRook) && !king_only) {

                // Rooks use first 4, bishops last 4, and queen all directions
                int startDir = (type == aBish) ? 4 : 0;
                int endDir   = (type == aRook) ? 4 : 8;

                for (int k = startDir; k < endDir; k++) {

                    // If pinned, only continue if the movemnet is on the same line/diagonal as pin
                    if (pinned) {
                        if ((rdirs[k] != pinned_rdirs[pinned_piece] || fdirs[k] != pinned_fdirs[pinned_piece])
                                && (-rdirs[k] != pinned_rdirs[pinned_piece] || -fdirs[k] != pinned_fdirs[pinned_piece])) {
                            continue;
                        }
                    }

                    int target_rank = rank + rdirs[k];
                    int target_file = file + fdirs[k];

                    // While on board
                    while (target_rank >= 0 && target_rank < 8 && target_file >= 0 && target_file < 8) {

                        // piece type on target_square
                        target_square = gs->board[target_rank][target_file];

                        // If free, record move
                        if (target_square == EMPTY) {
                            if (allowed_squares[target_rank][target_file]) { 
                              register_move(semi_quiet_moves, total_semi_quiet, rank, file, pInd, piece_side, target_rank, target_file, QUIET, type, STANDARD);
                            }
                        }
                        // else record capture if enemy, and break since we hit a piece
                        else {
                            if (allied_sign * target_square < 0) {
                                if (allowed_squares[target_rank][target_file]) {
                                    register_move(semi_capture_moves, total_semi_captures, rank, file, pInd, piece_side, target_rank, target_file, CAPTURE, type, STANDARD);
                                }
                            }
                            break;
                        }

                        // Continue walking
                        target_rank += rdirs[k];
                        target_file += fdirs[k];

                    }
                }
                continue;
            }


            // Knight jump
            if (type == aKni && !king_only) {
                // If pinned, dont allow any moves
                if (pinned) {
                    continue;
                }
                // Loop over all 8 possible jumps
                for (int k = 0; k < 8; k++) {

                    // Target square and type
                    target_rank = rank + krdirs[k];
                    target_file = file + kfdirs[k];

                    // If on board record moves
                    if (target_rank < 8 && target_rank >= 0 && target_file < 8 && target_file >= 0) {
                        target_square = gs->board[target_rank][target_file];

                        // If free, record quiet move
                        if (gs->board[target_rank][target_file] == EMPTY) {
                            if (allowed_squares[target_rank][target_file]) {
                                register_move(semi_quiet_moves, total_semi_quiet, rank, file, pInd, piece_side, target_rank, target_file, QUIET, type, STANDARD);
                            }
                        }
                        // Else if enemy record capture
                        else if (target_square * allied_sign < 0) {
                            if (allowed_squares[target_rank][target_file]) {
                                register_move(semi_capture_moves, total_semi_captures, rank, file, pInd, piece_side, target_rank, target_file, CAPTURE, type, STANDARD);
                            }
                        }
                    }

                }
                continue;
            }



            if (type == aPawn && !king_only) {
                for (int k = 0; k < 3; k++) {

                    if (pinned) {
                        int move_rdir = allied_sign;
                        int move_fdir = pfdirs[k];
                        int same_dir = (allied_sign != pinned_rdirs[pinned_piece] || move_fdir != pinned_fdirs[pinned_piece]);
                        int opp_dir  = (move_rdir != -pinned_rdirs[pinned_piece] || move_fdir != -pinned_fdirs[pinned_piece]);
                        if (same_dir && opp_dir) {
                            continue;
                        }
                    }
                    // Pawn directions, rank always up/down
                    target_file = file + pfdirs[k];
                    target_rank = rank + allied_sign;

                    // If target rank is the last rank, set promotion movetype
                    int movetype = (target_rank == 7 || target_rank == 0) ? PROMOTION : STANDARD;

                    // Check if on board after one step
                    if (target_rank < 8 && target_rank >= 0 && target_file < 8 && target_file >= 0) {

                        // Type
                        target_square = gs->board[target_rank][target_file];


                        // If free and fdir = 0 -> record quiet walk and check if two step walk allowed
                        if (target_square == EMPTY && pfdirs[k] == 0) {
                            if (allowed_squares[target_rank][target_file]) {
                                register_move(semi_quiet_moves, total_semi_quiet, rank, file, pInd, piece_side, target_rank, target_file, QUIET, type, movetype);
                            }

                            // Check if we are allowed one more step
                            if (((piece_side && (rank == 1)) || (!piece_side && rank == 6))) {
                                target_file = file;
                                target_rank = rank + 2 * allied_sign;

                                // Always on gs->board since df = 0 and we are on starting rank
                                target_square = gs->board[target_rank][target_file];
                                if (target_square == EMPTY) {
                                    if (allowed_squares[target_rank][target_file]) {
                                        register_move(semi_quiet_moves, total_semi_quiet, rank, file, pInd, piece_side, target_rank, target_file, QUIET, type, STANDARD);
                                    }
                                }
                            }
                        }
                        // Possible capture?
                        else if (target_square * allied_sign < 0 && pfdirs[k] != 0) {
                            if (allowed_squares[target_rank][target_file]) {
                                register_move(semi_capture_moves, total_semi_captures, rank, file, pInd, piece_side, target_rank, target_file, CAPTURE, type, movetype);
                            }
                        }

                        // En passant check
                        else if (gs->en_passant_square != -1
                                && target_rank == gs->en_passant_square/8
                                && target_file == gs->en_passant_square % 8) {
                            register_move(semi_capture_moves, total_semi_captures, rank, file, pInd, piece_side, target_rank, target_file, CAPTURE, type, EN_PASSANT);
                        }
                    }
                }
            }


            // King
            if (type == aKing) {

                for (int k = 0; k < 8; k++) {
                    // Same directions as sliding pieces, but only 1 step
                    target_rank = rank + rdirs[k];
                    target_file = file + fdirs[k];
                    // If on board
                    if (target_rank < 8 && target_rank >= 0 && target_file < 8 && target_file >= 0) {
                        target_square = gs->board[target_rank][target_file];

                        // If free, record quiet move
                        if (target_square == EMPTY) {
                            register_move(semi_quiet_moves, total_semi_quiet, rank, file, pInd, piece_side, target_rank, target_file, QUIET, type, STANDARD);
                        }
                        // Else if enemy record capture
                        else if (target_square * allied_sign < 0) {
                            register_move(semi_capture_moves, total_semi_captures, rank, file, pInd, piece_side, target_rank, target_file, CAPTURE, type, STANDARD);
                        }
                    }

                }
            }
        }
    }
}



int generate_legal_captures(struct GameState* gs, Move legal_moves[256]) {

    /*
     Finds and adds legal moves to gs->legal_moves.
     Returns total amount of legal moves found.
     */

    // Find pinned pieces
    int   pinned_pieces_square[16];
    int    total_pinned_pieces = 0;
    int            pinned_rdir[16];
    int            pinned_fdir[16];
    int checking_pieces_square[16];
    int  total_checking_pieces = 0;
    find_pinned_and_checking_pieces(gs, pinned_pieces_square, pinned_rdir, pinned_fdir, checking_pieces_square, &total_pinned_pieces, &total_checking_pieces, gs->board);


    int total_semi_captures = 0;
    int total_legal_moves = 0;
    Move semi_capture_moves[256];

    int king_rank;
    int king_file;
    int king_square;

    // Generate psuedo legal captures and iterate over them
    generate_pseudo_captures(&total_semi_captures, semi_capture_moves, gs, pinned_pieces_square, total_pinned_pieces, pinned_rdir, pinned_fdir);
    for (int SemiMove = 0; SemiMove < total_semi_captures; SemiMove++) {

        Move move = semi_capture_moves[SemiMove]; 

        int validate = 0;

        int square = get_from_square(move);
        int piece_type = gs->board[square / 8][square % 8];
        if (get_move_type(move) == EN_PASSANT || abs(piece_type) == KING) {
            validate = 1;
        }

        if (validate) {
            struct UndoInfo ui;
            play_move(move, gs, &ui);

            king_square = (gs->side_to_move == SIDE_BLACK) ? gs->white_king_square : gs->black_king_square;
            king_rank = king_square / 8;
            king_file = king_square % 8;

            if (is_king_safe(gs->board, king_rank, king_file, !gs->side_to_move)) {
                legal_moves[total_legal_moves] = move;
                total_legal_moves++;
            }

            unplay_move(move, gs, &ui);
        }
        else {
            legal_moves[total_legal_moves] = move;
            total_legal_moves++;
        }
    }

    return total_legal_moves;
}





int generate_legal_moves(struct GameState* gs, Move legal_moves[256]) {

    /*
     Finds and adds legal moves to gs->legal_moves.
     Returns total amount of legal moves found.
     */

    // Find pinned and checking pieces
    int   pinned_pieces_square[16];
    int   total_pinned_pieces = 0;
    int           pinned_rdir[16];
    int           pinned_fdir[16];
    int checking_pieces_square[16];
    int total_checking_pieces = 0;
    find_pinned_and_checking_pieces(gs, pinned_pieces_square, pinned_rdir, pinned_fdir, checking_pieces_square, &total_pinned_pieces, &total_checking_pieces, gs->board);


    int total_semi_quiet = 0;
    int total_semi_captures = 0;
    int total_legal_moves = 0;
    Move semi_quiet_moves[256];
    Move semi_capture_moves[256];


    int king_rank;
    int king_file;
    int king_square;

    // If not in check, generate psuedo legal moves
    if (total_checking_pieces == 0) {
        generate_pseudo_moves(&total_semi_quiet, &total_semi_captures, semi_quiet_moves, semi_capture_moves, gs, pinned_pieces_square, total_pinned_pieces, pinned_rdir, pinned_fdir);
        for (int SemiMove = 0; SemiMove < total_semi_quiet + total_semi_captures; SemiMove++) {

            Move move = (SemiMove < total_semi_captures) ? semi_capture_moves[SemiMove] : semi_quiet_moves[SemiMove - total_semi_captures];

            int validate = 0;
                
            int square = get_from_square(move);
            int piece_type = gs->board[square / 8][square % 8];
            if (get_move_type(move) == EN_PASSANT || abs(piece_type) == KING) {
                validate = 1;
            }
            if (validate) {

                struct UndoInfo ui;
                play_move(move, gs, &ui);

                king_square = (gs->side_to_move == SIDE_BLACK) ? gs->white_king_square : gs->black_king_square;
                king_rank = king_square / 8;
                king_file = king_square % 8;

                if (is_king_safe(gs->board, king_rank, king_file, !gs->side_to_move)) {
                    legal_moves[total_legal_moves] = move;
                    total_legal_moves++;
                }

                unplay_move(move, gs, &ui);
            }
            else {
                legal_moves[total_legal_moves] = move;
                total_legal_moves++;
            }   
        }
    }
    else { // If in check generate pseudo evasions

        generate_pseudo_evasions(&total_semi_quiet, &total_semi_captures, semi_quiet_moves, semi_capture_moves, gs, pinned_pieces_square, total_pinned_pieces, pinned_rdir, pinned_fdir, checking_pieces_square, total_checking_pieces);

        for (int SemiMove = 0; SemiMove < total_semi_quiet + total_semi_captures; SemiMove++) {

            Move move = (SemiMove < total_semi_captures) ? semi_capture_moves[SemiMove] : semi_quiet_moves[SemiMove - total_semi_captures];

            int validate = 0;

            int square = get_from_square(move);
            int piece_type = gs->board[square / 8][square % 8];
            if (get_move_type(move) == EN_PASSANT || abs(piece_type) == KING) {
                validate = 1;
            }
            if (validate) {

                struct UndoInfo ui;
                play_move(move, gs, &ui);

                king_square = (gs->side_to_move == SIDE_BLACK) ? gs->white_king_square : gs->black_king_square;
                king_rank = king_square / 8;
                king_file = king_square % 8;

                if (is_king_safe(gs->board, king_rank, king_file, !gs->side_to_move)) {
                    legal_moves[total_legal_moves] = move;
                    total_legal_moves++;
                }

                unplay_move(move, gs, &ui);
            }
            else {
                legal_moves[total_legal_moves] = move;
                total_legal_moves++;
            }
        }
    }

    return total_legal_moves;
}




int is_move_legal(struct GameState* gs, char input[6], uint32_t* matched_legal_move) {

    /* Returns a pointer to a legal move if found */


    // Convert input string to a uint32_t
    uint32_t input_move = {0};
    input_move = uci_string_to_move(input);

    // Find all legal moves to compare to
    uint32_t legal_moves[256];
    int total = generate_legal_moves(gs, legal_moves);



    for (int i = 0; i < total; i++) {

        uint32_t legal_move = legal_moves[i];

        if (get_move_type(legal_move) == PROMOTION) {
            if (get_from_square(legal_move) == get_from_square(input_move)
                && get_to_square(legal_move) == get_to_square(input_move)
            && get_promotion_type(legal_move) == get_promotion_type(input_move)) {

                // Move found -> return pointer
                *matched_legal_move = legal_move;
                return 1;
            }

        }
        else if (get_move_type(legal_move) == SHORT_CASTLE) {
            if (get_from_square(legal_move) == get_from_square(input_move)
                && get_to_square(legal_move) - 1 == get_to_square(input_move)) {

                // Move found -> return pointer
                *matched_legal_move = legal_move;
                return 1;
                }
        }
        else if (get_move_type(legal_move) == LONG_CASTLE) {
            if (get_from_square(legal_move) == get_from_square(input_move)
                && get_to_square(legal_move) + 2 == get_to_square(input_move)) {

                // Move found -> return pointer
                *matched_legal_move = legal_move;
                return 1;
                }
        }
        else {
            if (get_from_square(legal_move) == get_from_square(input_move)
                    && get_to_square(legal_move) == get_to_square(input_move)) {

                    // Move found -> return pointer
                    *matched_legal_move = legal_move;
                    return 1;
            }
        }
    }

    return 0;
}


int is_king_safe(int board[8][8], int rank, int file, int piece_side) {

    /* Checks if a piece is attacking the king*/


    int enemy_sign = (piece_side == SIDE_WHITE) ? -1 : 1;
    int ePawn  = enemy_sign * PAWN;
    int eKni   = enemy_sign * KNIGHT;
    int eBish  = enemy_sign * BISHOP;
    int eRook  = enemy_sign * ROOK;
    int eQueen = enemy_sign * QUEEN;
    int eKing  = enemy_sign * KING;

    // Straight and diagonal lines
    static const int rdirs[8] = {1, -1, 0, 0, 1, 1, -1, -1};
    static const int fdirs[8] = {0, 0, 1, -1, 1, -1, -1, 1};

    for (int k = 0; k < 8; k++) {
        int target_rank = rank + rdirs[k];
        int target_file = file + fdirs[k];

        while (target_rank >= 0 && target_rank < 8 && target_file >= 0 && target_file < 8) {

            int piece = board[target_rank][target_file];

            // If piece
            if (piece != 0) {
                // Straight lines
                if (k < 4) {
                    if (piece == eRook || piece == eQueen) return 0;
                }
                // Diagonal lines
                else {
                    if (piece == eBish || piece == eQueen) return 0;
                }
                // Not enemy, safe
                break;
            }
            target_rank += rdirs[k];
            target_file += fdirs[k];
        }
    }

    // Knight jumps
    static const int krdirs[8] = {2, 2, -2, -2, 1, -1, 1, -1};
    static const int kfdirs[8] = {1, -1, 1, -1, 2, 2, -2, -2};

    for (int k = 0; k < 8; k++) {
        int target_rank = rank + krdirs[k];
        int target_file = file + kfdirs[k];
        if (target_rank >= 0 && target_rank < 8 && target_file >= 0 && target_file < 8) {
            if (board[target_rank][target_file] == eKni) return 0;
        }
    }


    // Check diagonals infrom of king for pawns
    int pawn_dir = piece_side ? 1 : -1;
    int target_rank = rank + pawn_dir;

    if (target_rank >= 0 && target_rank < 8) {
        if (file - 1 >= 0 && board[target_rank][file - 1] == ePawn) return 0;
        if (file + 1 < 8 && board[target_rank][file + 1] == ePawn) return 0;
    }

    // king checks straight and diagonal lines but only one step
    for (int k = 0; k < 8; k++) {
        int target_rank = rank + rdirs[k];
        int target_file = file + fdirs[k];
        if (target_rank >= 0 && target_rank < 8 && target_file >= 0 && target_file < 8) {
            if (board[target_rank][target_file] == eKing) return 0;
        }
    }

    // King is chilling
    return 1;
}




