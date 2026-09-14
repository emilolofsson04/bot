#ifndef EVAL_H
#define EVAL_H

#include "structs.h"
#include "movegen.h"



static inline int Eval(struct GameState* gs, struct Evalboards* eb) {

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

    int wBishops = 0;
    int bBishops = 0;

    for (int i = 0; i < 16; i++) {
       if (wPieces[i].alive) {
           int type = wPieces[i].type;
            wsum += eb->PieceValues[type];
            if (type == 1) {
                wsum += eb->Pawnboard[wPieces[i].rank][wPieces[i].file];
                white_pawn_files[wPieces[i].file]++;
                if (white_pawn_files[wPieces[i].file] > 1) {
                    wsum -= doubled_pawn;
                }
                
            }
            if (type == 2) {
                wsum += eb->Knightboard[wPieces[i].rank][wPieces[i].file];

            }
            if (type == 3) {
                wsum += eb->Bishopboard[wPieces[i].rank][wPieces[i].file];
                wBishops++;
                if (wBishops == 2) wsum += 10;

            }
            if (type == 4) {
                wsum += eb->Rookboard[wPieces[i].rank][wPieces[i].file];
            }
            if (type == 5) {
                wsum += eb->Queenboard[wPieces[i].rank][wPieces[i].file];
            }

            if (type == 6) {
                if (wPieces[i].rank == 0) {
                    wsum += 20;
                    if (wPieces[i].file > 5 || wPieces[i].file <= 2) {
                        wsum += 20;
                    }
                }
            }
       }
       if (bPieces[i].alive) {
           int type = bPieces[i].type;
            bsum += eb->PieceValues[abs(type)];
            if (type == -1) {
                bsum += eb->Pawnboard[7 - (bPieces[i].rank)][bPieces[i].file];

                black_pawn_files[bPieces[i].file]++;
                if (black_pawn_files[bPieces[i].file] > 1) {
                    bsum -= doubled_pawn;
                }
            }
            if (type == -2) {
                bsum += eb->Knightboard[7 - (bPieces[i].rank)][bPieces[i].file];

            }
            if (type == -3) {
                bsum += eb->Bishopboard[7 - (bPieces[i].rank)][bPieces[i].file];
                bBishops++;
                if (bBishops == 2) bsum += 10;

            }
            if (type == -4) {
                bsum += eb->Rookboard[7 - (bPieces[i].rank)][bPieces[i].file];
            }
            if (type == -5) {
                bsum += eb->Queenboard[7 - (bPieces[i].rank)][bPieces[i].file];
            }
            if (type == -6) {
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
    
    /*
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
    */
    if ((wsum - bsum) == 0) return 1;
    return who2play * (wsum - bsum);

}

#endif
