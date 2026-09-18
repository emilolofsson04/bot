#ifndef EVAL_H
#define EVAL_H

#include "types.h"
#include "pst.h"

static inline void initiate_evaluation(struct GameState* Game) {

    Game->eval = (struct Evaluation){0};
    struct piece* wPieces = Game->white_pieces;
    struct piece* bPieces = Game->black_pieces;

    for (int i = 0; i < 16; i++) {
       if (wPieces[i].alive) {
           int type = wPieces[i].type;
            Game->eval.white_material += piece_values[type];
            Game->eval.white_phase += game_stage_values[type];

            Game->eval.mg_evaluation += EarlyPST[type][wPieces[i].rank][wPieces[i].file];
            Game->eval.eg_evaluation  += LatePST[type][wPieces[i].rank][wPieces[i].file];
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
            Game->eval.black_material += piece_values[type];
            Game->eval.black_phase += game_stage_values[type];

            Game->eval.mg_evaluation -= EarlyPST[type][rank][bPieces[i].file];
            Game->eval.eg_evaluation  -= LatePST[type][rank][bPieces[i].file];
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
    if (phase > MAX_PHASE) phase = MAX_PHASE;

    int mg = Game->eval.mg_evaluation;
    int eg = Game->eval.eg_evaluation;
    int material_difference = Game->eval.white_material - Game->eval.black_material;
    int score = material_difference + ((mg * phase) + (eg * (MAX_PHASE - phase)) + MAX_PHASE / 2) / MAX_PHASE;

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

static inline int Eval(struct GameState* Game) {

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

    struct piece* wPieces = Game->white_pieces;
    struct piece* bPieces = Game->black_pieces;

    int who2play = (Game->side_to_move == SIDE_WHITE) ? 1 : -1;

    int white_pawn_files[8] = {0};
    int black_pawn_files[8] = {0};
    int doubled_pawn = DOUBLED_PAWN_PENALTY;

    int wBishops = 0;
    int bBishops = 0;

    int phase = 0;

    for (int i = 0; i < 16; i++) {
       if (wPieces[i].alive) {
           int type = wPieces[i].type;
            w_material_sum += piece_values[type];
            phase += game_stage_values[type];

            w_early_sum += EarlyPST[type][wPieces[i].rank][wPieces[i].file];
            w_late_sum  += LatePST[type][wPieces[i].rank][wPieces[i].file];
            if (type == PAWN) {
                white_pawn_files[wPieces[i].file]++;
                if (white_pawn_files[wPieces[i].file] > 1) {
                    w_final_sum -= doubled_pawn;
                }
                
            }
            if (type == BISHOP) {
                wBishops++;
                if (wBishops == 2) w_final_sum += BISHOP_PAIR_BONUS;

            }
       }
       if (bPieces[i].alive) {
            int type = abs(bPieces[i].type);
            int rank = 7 - bPieces[i].rank;
            b_material_sum += piece_values[abs(type)];
            phase += game_stage_values[abs(type)];

            b_early_sum += EarlyPST[type][rank][bPieces[i].file];
            b_late_sum  += LatePST[type][rank][bPieces[i].file];

            if (type == PAWN) {
                black_pawn_files[bPieces[i].file]++;
                if (black_pawn_files[bPieces[i].file] > 1) {
                    b_final_sum -= doubled_pawn;
                }

            }
            if (type == BISHOP) {
                bBishops++;
                if (bBishops == 2) b_final_sum += BISHOP_PAIR_BONUS;
            }
       }
    }

    if (phase > MAX_PHASE) phase = MAX_PHASE;
    w_final_sum += w_material_sum;

    w_final_sum += (w_early_sum * phase + w_late_sum * (MAX_PHASE - phase) + MAX_PHASE / 2) / MAX_PHASE;

    b_final_sum += b_material_sum;

    b_final_sum += (b_early_sum * phase + b_late_sum * (MAX_PHASE - phase) + MAX_PHASE / 2) / MAX_PHASE;


    if ((w_final_sum - b_final_sum) == DRAW_SCORE) return 1;
    return who2play * (w_final_sum - b_final_sum);

}

#endif
