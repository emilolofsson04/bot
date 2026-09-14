#ifndef EVAL_H
#define EVAL_H

#include "structs.h"
#include "movegen.h"



static inline int Eval(struct GameState* gs, struct EvalBoards* eb) {

    /*
     Evaluates a postion from scratch,
     returning a int value of the difference
     in value for white and black.
     Goal is to make this based on previous
     evals, to lessen the amount of
     computation needed.
     */

    int wsum = 0;
    int bsum = 0;
    struct piece* wPieces = gs->white_pieces;
    struct piece* bPieces = gs->black_pieces;

    int who2play = (gs->side_to_move == SIDE_WHITE) ? 1 : -1;

    int white_pawn_files[8] = {0};
    int black_pawn_files[8] = {0};
    int doubled_pawn = 5;

    for (int i = 0; i < 16; i++) {
       if (wPieces[i].alive) {
            wsum += 100 * wPieces[i].value;
            if (wPieces[i].type == 1) {
                wsum += eb->PawnBoard[wPieces[i].rank][wPieces[i].file];
                white_pawn_files[wPieces[i].file]++;
                if (white_pawn_files[wPieces[i].file] > 1) {
                    wsum -= doubled_pawn;
                }
                
            }
            if (wPieces[i].type == 2) {
                wsum += eb->KnightBoard[wPieces[i].rank][wPieces[i].file];

            }
            if (wPieces[i].type == 3) {
                wsum += eb->BishopBoard[wPieces[i].rank][wPieces[i].file];

            }
            if (wPieces[i].type == 4) {
                wsum += eb->RookBoard[wPieces[i].rank][wPieces[i].file];
            }
            if (wPieces[i].type == 5) {
                wsum += eb->QueenBoard[wPieces[i].rank][wPieces[i].file];
            }

            if (wPieces[i].type == 6) {
                if (wPieces[i].rank == 0) {
                    wsum += 20;
                    if (wPieces[i].file > 5 || wPieces[i].file <= 2) {
                        wsum += 20;
                    }
                }
            }
       }
       if (bPieces[i].alive) {
            bsum += 100 * bPieces[i].value;
            if (bPieces[i].type == -1) {
                bsum += eb->PawnBoard[7 - (bPieces[i].rank)][bPieces[i].file];

                black_pawn_files[bPieces[i].file]++;
                if (black_pawn_files[bPieces[i].file] > 1) {
                    bsum -= doubled_pawn;
                }
            }
            if (bPieces[i].type == -2) {
                bsum += eb->KnightBoard[7 - (bPieces[i].rank)][bPieces[i].file];

            }
            if (bPieces[i].type == -3) {
                bsum += eb->BishopBoard[7 - (bPieces[i].rank)][bPieces[i].file];

            }
            if (bPieces[i].type == -4) {
                bsum += eb->RookBoard[7 - (bPieces[i].rank)][bPieces[i].file];
            }
            if (bPieces[i].type == -5) {
                bsum += eb->QueenBoard[7 - (bPieces[i].rank)][bPieces[i].file];
            }
            if (bPieces[i].type == -6) {
                if (bPieces[i].rank == 7) {
                    bsum += 20;
                    if (bPieces[i].file > 5 || bPieces[i].file <= 2) {
                        bsum += 20;
                    }
                }
            }

       }
    }

    // Add some bonuses
    
    for (int file = 0; file < 8; file++) {
        if (file == 0) {
            if (white_pawn_files[file]
                 && white_pawn_files[file + 1]) {
                 wsum += 5;

            }
            if (black_pawn_files[file]
                 && black_pawn_files[file + 1]) {
                 bsum += 5;

            }
            continue;

        }
        if (file == 7) {
            if (white_pawn_files[file]
                 && white_pawn_files[file - 1]) {
                 wsum += 5;

            }
            if (black_pawn_files[file]
                 && black_pawn_files[file - 1]) {
                 bsum += 5;

            }
            continue;
        }
        if (white_pawn_files[file]
             && (white_pawn_files[file - 1] || white_pawn_files[file + 1])) {
             wsum += 5;
        
        }
        if (black_pawn_files[file]
             && (black_pawn_files[file - 1] || black_pawn_files[file + 1])) {
             bsum += 5;

        }
    }
    return who2play * (wsum - bsum);

}

#endif
