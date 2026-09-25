#ifndef EVAL_H
#define EVAL_H

#include "types.h"
#include "pst.h"

extern uint64_t passed_pawn_masks[2][64];

void init_evaluation_masks(void);

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
                Game->eval.white_pawns ^= (1ULL << (8 * wPieces[i].rank + wPieces[i].file));

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
                Game->eval.black_pawns ^= (1ULL << (8 * bPieces[i].rank + bPieces[i].file));
            }
            if (type == BISHOP) {
                Game->eval.black_bishops++;
            }
       }
    }
}
static const int knight_mobility[8][8] = {
    { 2, 3, 4, 4, 4, 4, 3, 2 },
    { 3, 4, 6, 6, 6, 6, 4, 3 },
    { 4, 6, 8, 8, 8, 8, 6, 4 },
    { 4, 6, 8, 8, 8, 8, 6, 4 },
    { 4, 6, 8, 8, 8, 8, 6, 4 },
    { 4, 6, 8, 8, 8, 8, 6, 4 },
    { 3, 4, 6, 6, 6, 6, 4, 3 },
    { 2, 3, 4, 4, 4, 4, 3, 2 }
};

static const int dr[8] = {
     1,      0,     -1,      0,      1,     -1,     -1,      1
};

static const int df[8] = {
     0,      1,      0,     -1,      1,      1,     -1,     -1
};

static inline int piece_mobility(struct GameState* Game, struct piece Piece) {

    int piece_type = abs(Piece.type);
    if (piece_type == PAWN || piece_type == KING) return 0;
    
    int piece_rank = Piece.rank;
    int piece_file = Piece.file;

    if (piece_type == KNIGHT) return 2 * knight_mobility[piece_rank][piece_file];

    int mobility_score = 0;
    int start_index = (piece_type == BISHOP) ? 4 : 0; 
    int end_index = (piece_type == ROOK) ? 4 : 8; 

    int weight = 1;
    if (piece_type == ROOK) weight = 3;
    if (piece_type == BISHOP) weight = 5;
    for (int dir = start_index; dir < end_index; dir++) {
        int r = piece_rank + dr[dir];
        int f = piece_file + df[dir];

        while (r >= 0 && r < 8 && f >= 0 && f < 8 && Game->board[r][f] == EMPTY) {

            mobility_score += weight;
            r += dr[dir];
            f += df[dir];
        }
    }
    return mobility_score;
}
static inline int mobility_evaluation(struct GameState* Game) {
    int mobility_score = 0;
    for (int i = 0; i < 16; i++) {
        
        struct piece wpiece = Game->white_pieces[i]; 
        if (wpiece.alive) mobility_score += piece_mobility(Game, wpiece);

        struct piece bpiece = Game->black_pieces[i]; 
        if (bpiece.alive) mobility_score -= piece_mobility(Game, bpiece);
    }
    return mobility_score;
}
static inline int check_open_squares(int piece_rank, int piece_file, int weight, struct GameState* Game) {

    int mobility_score = 0;
    for (int dir = 0; dir < 8; dir++) {
        int r = piece_rank + dr[dir];
        int f = piece_file + df[dir];

        while (r >= 0 && r < 8 && f >= 0 && f < 8 && Game->board[r][f] == EMPTY) {

            mobility_score += weight;
            r += dr[dir];
            f += df[dir];
        }
    }
    return -mobility_score;


}
static inline int king_mobility_danger(struct GameState* Game) {

    int wking_square = Game->white_king_square;
    int bking_square = Game->black_king_square;

    int w_king_mobility = check_open_squares(wking_square /8, wking_square % 8, 4, Game);
    int b_king_mobility = check_open_squares(bking_square /8, bking_square % 8, 4, Game);
    return w_king_mobility - b_king_mobility;

}
static inline int passed_pawns(int side, uint64_t allied_pawns, uint64_t enemy_pawns) {

    int total_bonus = 0;
        
    static const int passed_bonus[8] = { 0, 5, 10, 20, 45, 90, 160, 0 };

    while (allied_pawns) {
        int square = __builtin_ctzll(allied_pawns);

        if (!(passed_pawn_masks[side][square] & enemy_pawns)) {
            int relative_rank = (side == SIDE_WHITE) ? (square / 8) : (7 - (square / 8));
            total_bonus += passed_bonus[relative_rank];
        }

        allied_pawns &= allied_pawns - 1;
    }
    return total_bonus;
}

static inline int evaluate_position(struct GameState* Game, int alpha, int beta) {

    int phase = Game->eval.white_phase + Game->eval.black_phase;
    if (phase > MAX_PHASE) phase = MAX_PHASE;

    int mg = Game->eval.mg_evaluation;
    int eg = Game->eval.eg_evaluation;
    int material_difference = Game->eval.white_material - Game->eval.black_material;
    int score = material_difference + ((mg * phase) + (eg * (MAX_PHASE - phase)) + MAX_PHASE / 2) / MAX_PHASE;

    if (Game->eval.white_bishops >= 2) score += BISHOP_PAIR_BONUS;
    if (Game->eval.black_bishops >= 2) score -= BISHOP_PAIR_BONUS;

    static const uint64_t FILE_A = 0x0101010101010101ULL;

    for (int f = 0; f < 8; f++) {
        uint64_t file_mask = FILE_A << f;

        int w_count = __builtin_popcountll(Game->eval.white_pawns & file_mask);
        int b_count = __builtin_popcountll(Game->eval.black_pawns & file_mask);

        if (w_count > 1) score -= (w_count - 1) * DOUBLED_PAWN_PENALTY;
        if (b_count > 1) score += (b_count - 1) * DOUBLED_PAWN_PENALTY;
    }

    int passed_pawns_score = passed_pawns(SIDE_WHITE, Game->eval.white_pawns, Game->eval.black_pawns) - passed_pawns(SIDE_BLACK, Game->eval.black_pawns, Game->eval.white_pawns);

    score += ((MAX_PHASE - phase)* passed_pawns_score) / MAX_PHASE;

    score = (Game->side_to_move == SIDE_WHITE) ? score : -score;



    if (score < alpha - LAZY_MARGIN) return score + LAZY_MARGIN;
    else if (score > beta + LAZY_MARGIN) return score - LAZY_MARGIN;

    score += (Game->side_to_move == SIDE_WHITE) ? (phase * king_mobility_danger(Game)) / MAX_PHASE : - (phase * king_mobility_danger(Game)) / MAX_PHASE;

    score += (Game->side_to_move == SIDE_WHITE) ? mobility_evaluation(Game): - mobility_evaluation(Game);

    return score;
    
}


#endif
