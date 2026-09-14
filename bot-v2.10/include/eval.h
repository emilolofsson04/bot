#ifndef EVAL_H
#define EVAL_H

#include "structs.h"

static inline void initiate_evaluation(struct GameState* Game, struct Evalboards* eb) {

    Game->eval = (struct Evaluation){0};
    struct piece* wPieces = Game->white_pieces;
    struct piece* bPieces = Game->black_pieces;

    for (int i = 0; i < 16; i++) {
       if (wPieces[i].alive) {
           int type = wPieces[i].type;
            Game->eval.white_material += eb->PieceValues[type];
            Game->eval.white_phase += eb->game_stage_values[type];

            Game->eval.mg_evaluation += eb->EarlyPST[type][wPieces[i].rank][wPieces[i].file];
            Game->eval.eg_evaluation  += eb->LatePST[type][wPieces[i].rank][wPieces[i].file];
            if (type == PAWN) {
                Game->eval.white_pawn_files += (1u << (4 * wPieces[i].file));

            }
            if (type == BISHOP) {
                Game->eval.white_bishops++;
            }
       }
       if (bPieces[i].alive) {
            int type = abs(bPieces[i].type);
            int rank = 7 - bPieces[i].rank;
            Game->eval.black_material += eb->PieceValues[type];
            Game->eval.black_phase += eb->game_stage_values[type];

            Game->eval.mg_evaluation -= eb->EarlyPST[type][rank][bPieces[i].file];
            Game->eval.eg_evaluation  -= eb->LatePST[type][rank][bPieces[i].file];
            if (type == PAWN) {
                Game->eval.black_pawn_files += (1u << (4 * bPieces[i].file));

            }
            if (type == BISHOP) {
                Game->eval.black_bishops++;
            }
       }
    }
}

static inline int evaluate_position(struct GameState* Game) {

    int phase = Game->eval.white_phase + Game->eval.black_phase;
    if (phase > 24) phase = 24;

    int mg = Game->eval.mg_evaluation;
    int eg = Game->eval.eg_evaluation;
    int material_difference = Game->eval.white_material - Game->eval.black_material;
    int score = material_difference + ((mg * phase) + (eg * (24 - phase)) + 12) / 24;

    if (Game->eval.white_bishops >= 2) score += BISHOP_PAIR_BONUS;
    if (Game->eval.black_bishops >= 2) score -= BISHOP_PAIR_BONUS;

    uint32_t w_pawns = Game->eval.white_pawn_files;
    uint32_t b_pawns = Game->eval.black_pawn_files;
    for (int f = 0; f < 8; f++) {
        int w_count = (w_pawns >> (f << 2)) & 15;
        int b_count = (b_pawns >> (f << 2)) & 15;
        if (w_count > 1) score -= (w_count - 1) * DOUBLED_PAWN_PENALTY;
        if (b_count > 1) score += (b_count - 1) * DOUBLED_PAWN_PENALTY;
    }

    return (Game->side_to_move == SIDE_WHITE) ? score : -score;
    
}

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

            w_early_sum += eb->EarlyPST[type][wPieces[i].rank][wPieces[i].file];
            w_late_sum  += eb->LatePST[type][wPieces[i].rank][wPieces[i].file];
            if (type == PAWN) {
                white_pawn_files[wPieces[i].file]++;
                if (white_pawn_files[wPieces[i].file] > 1) {
                    w_final_sum -= doubled_pawn;
                }
                
            }
            if (type == BISHOP) {
                wBishops++;
                if (wBishops == 2) w_final_sum += 10;

            }
       }
       if (bPieces[i].alive) {
            int type = abs(bPieces[i].type);
            int rank = 7 - bPieces[i].rank;
            b_material_sum += eb->PieceValues[abs(type)];
            phase += eb->game_stage_values[abs(type)];

            b_early_sum += eb->EarlyPST[type][rank][bPieces[i].file];
            b_late_sum  += eb->LatePST[type][rank][bPieces[i].file];

            if (type == PAWN) {
                black_pawn_files[bPieces[i].file]++;
                if (black_pawn_files[bPieces[i].file] > 1) {
                    b_final_sum -= doubled_pawn;
                }

            }
            if (type == BISHOP) {
                bBishops++;
                if (bBishops == 2) b_final_sum += 10;
            }
       }
    }

    if (phase > 24) phase = 24;
    w_final_sum += w_material_sum;

    w_final_sum += (w_early_sum * phase + w_late_sum * (24 - phase) + 12) / 24;

    b_final_sum += b_material_sum;

    b_final_sum += (b_early_sum * phase + b_late_sum * (24 - phase) + 12) / 24;


    
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
    return who2play * (w_final_sum - b_final_sum);

}

#endif
