#pragma once 

#define MAX_DEPTH                  64
#define MAX_MOVES                 256 

#define INPUT_BUFFER_SIZE        8192
#define ENGINE_MAX_DEPTH           30

#define ASPIRATION_INITIAL_DELTA   60
#define ASPIRATION_MIN_DEPTH        5

#define NMP_BASE_REDUCTION          2
#define NMP_MIN_DEPTH               3

#define LMR_MIN_MOVE                4
#define LMR_MIN_DEPTH               5
#define LMR_BASE                  0.5
#define LMR_MULTIPLIER              1
#define LMR_DIVISOR                 2

#define TIME_MARGIN               1.2
#define TIME_CHECK_INTERVAL      4096

#define PAWN_PHASE_WEIGHT           0
#define KNIGHT_PHASE_WEIGHT         1
#define BISHOP_PHASE_WEIGHT         1
#define ROOK_PHASE_WEIGHT           2
#define QUEEN_PHASE_WEIGHT          4
#define MAX_PHASE                  24

#define BISHOP_PAIR_BONUS          10
#define DOUBLED_PAWN_PENALTY        5

#define PAWN_VALUE_CP             100
#define KNIGHT_VALUE_CP           310
#define BISHOP_VALUE_CP           320
#define ROOK_VALUE_CP             500
#define QUEEN_VALUE_CP            900
#define KING_VALUE_CP            2000

#define TIME_REMAINING_DIVISOR     30
#define TIME_INCRAMENT_DIVISOR      2

#define TT_SIZE             (1u << 23)
