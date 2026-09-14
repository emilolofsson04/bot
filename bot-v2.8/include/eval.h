#ifndef EVAL_H
#define EVAL_H

#include "structs.h"


static inline int Eval(struct GameState* gs, struct Evalboards* eb) {

    /*
     Evaluates a postion from scratch,
     returning a int value of the difference
     in value for white and black.
     Goal is to make this based on previous
     evals, to lessen the amount of
     computation needed.
     */

    int w_material_sum = 0;
    int b_material_sum = 0;

    int w_early_sum = 0;
    int b_early_sum = 0;

    int w_late_sum = 0;
    int b_late_sum = 0;

    int w_final_sum = 0;
    int b_final_sum = 0;

    struct piece* wPieces = gs->white_pieces;
    struct piece* bPieces = gs->black_pieces;

    int who2play = (gs->side_to_move == SIDE_WHITE) ? 1 : -1;

    int white_pawn_files[8] = {0};
    int black_pawn_files[8] = {0};
    int doubled_pawn = 5;

    int wBishops = 0;
    int bBishops = 0;

    int phase = 0;

    for (int i = 0; i < 16; i++) {
       if (wPieces[i].alive) {
           int type = wPieces[i].type;
            w_material_sum += eb->PieceValues[type];
            phase += eb->game_stage_values[type];
            if (type == 1) {
                w_final_sum += eb->Pawnboard[wPieces[i].rank][wPieces[i].file];
                white_pawn_files[wPieces[i].file]++;
                if (white_pawn_files[wPieces[i].file] > 1) {
                    w_final_sum -= doubled_pawn;
                }
                
            }
            if (type == 2) {
                w_final_sum += eb->Knightboard[wPieces[i].rank][wPieces[i].file];

            }
            if (type == 3) {
                w_final_sum += eb->Bishopboard[wPieces[i].rank][wPieces[i].file];
                wBishops++;
                if (wBishops == 2) w_final_sum += 10;

            }
            if (type == 4) {
                w_final_sum += eb->Rookboard[wPieces[i].rank][wPieces[i].file];
            }
            if (type == 5) {
                w_early_sum += eb->QueenEarlyboard[wPieces[i].rank][wPieces[i].file];
                w_late_sum += eb->QueenLateboard[wPieces[i].rank][wPieces[i].file];
            }

            if (type == 6) {
                w_early_sum += eb->KingEarlyboard[wPieces[i].rank][wPieces[i].file];
                w_late_sum += eb->KingLateboard[wPieces[i].rank][wPieces[i].file];
            }
       }
       if (bPieces[i].alive) {
           int type = bPieces[i].type;
            b_material_sum += eb->PieceValues[abs(type)];
            phase += eb->game_stage_values[abs(type)];
            if (type == -1) {
                b_final_sum += eb->Pawnboard[7 - (bPieces[i].rank)][bPieces[i].file];

                black_pawn_files[bPieces[i].file]++;
                if (black_pawn_files[bPieces[i].file] > 1) {
                    b_final_sum -= doubled_pawn;
                }
            }
            if (type == -2) {
                b_final_sum += eb->Knightboard[7 - (bPieces[i].rank)][bPieces[i].file];

            }
            if (type == -3) {
                b_final_sum += eb->Bishopboard[7 - (bPieces[i].rank)][bPieces[i].file];
                bBishops++;
                if (bBishops == 2) b_final_sum += 10;

            }
            if (type == -4) {
                b_final_sum += eb->Rookboard[7 - (bPieces[i].rank)][bPieces[i].file];
            }
            if (type == -5) {
                b_early_sum += eb->QueenEarlyboard[7 - (bPieces[i].rank)][bPieces[i].file];
                b_late_sum += eb->QueenLateboard[7 - (bPieces[i].rank)][bPieces[i].file];
            }
            if (type == -6) {
                b_early_sum += eb->KingEarlyboard[7 - (bPieces[i].rank)][bPieces[i].file];
                b_late_sum += eb->KingLateboard[7 - (bPieces[i].rank)][bPieces[i].file];
            }

       }
    }

    w_final_sum += w_material_sum;

    w_final_sum += (w_early_sum * phase + w_late_sum * (24 - phase)) / 24;

    b_final_sum += b_material_sum;

    b_final_sum += (b_early_sum * phase + b_late_sum * (24 - phase)) / 24;


    
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
    if ((w_final_sum - b_final_sum) == DRAW_SCORE) return 1;
    return who2play * (w_final_sum - b_final_sum);

}

#endif
