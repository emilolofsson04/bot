#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>
#include <raylib.h>

// enums
enum Side { SIDE_BLACK = 0, SIDE_WHITE = 1 };
enum Piece { EMPTY = 0, PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6 };
enum MoveType { STANDARD = 0, EN_PASSANT = 1, SHORT_CASTLE = 2, LONG_CASTLE = 3, PROMOTION = 4 };
enum CaptureFlag { QUIET = 0, CAPTURE = 1 };
enum CastlingRights { WK = 1, WQ = 2, BK = 4, BQ = 8 };
enum Shift { FROM_SHIFT = 0, TO_SHIFT = 6, PIECE_INDEX_SHIFT = 12, MOVE_TYPE_SHIFT = 16, CAPTURE_SHIFT = 19, PROMOTION_TYPE_SHIFT = 20, CAPTURED_INDEX_SHIFT = 23 } ;

enum Mask { FROM_MASK = 63 << FROM_SHIFT, TO_MASK = 63 << TO_SHIFT, PIECE_INDEX_MASK = 15 << PIECE_INDEX_SHIFT, MOVE_TYPE_MASK = 7 << MOVE_TYPE_SHIFT, CAPTURE_MASK = 1 << CAPTURE_SHIFT, PROMOTION_TYPE_MASK = 7 << PROMOTION_TYPE_SHIFT, CAPTURED_INDEX_MASK = 15 << CAPTURED_INDEX_SHIFT };

typedef uint32_t Move;
static inline int get_from_square(Move move) {
    return (move & FROM_MASK) >> FROM_SHIFT;
}

static inline int get_to_square(Move move) {
    return (move & TO_MASK) >> TO_SHIFT;
}

static inline int get_piece_index(Move move) {
    return (move & PIECE_INDEX_MASK) >> PIECE_INDEX_SHIFT;
}

static inline int get_move_type(Move move) {
    return (move & MOVE_TYPE_MASK) >> MOVE_TYPE_SHIFT;
}

static inline int get_capture_flag(Move move) {
    return (move & CAPTURE_MASK) >> CAPTURE_SHIFT;
}

static inline int get_promotion_type(Move move) {
    return (move & PROMOTION_TYPE_MASK) >> PROMOTION_TYPE_SHIFT;
}

static inline int get_captured_index(Move move) {
    return (move & CAPTURED_INDEX_MASK) >> CAPTURED_INDEX_SHIFT;
}


struct piece {
    int type;
    int colour;
    int rank;
    int file;
    int alive;
    int value;
};

struct GameState {
    int Board[8][8];
    int Index_Board[8][8];
    int halfmove_clock;
    int ply;
    struct piece white_pieces[16];
    struct piece black_pieces[16];
    int side_to_move;
    int iteration;
    uint64_t zobrist_hash_3fold_history[512];
    uint64_t zobrist_hash_3fold;
    int game;
    uint8_t castling_rights;
    int en_passant_square;
    int white_king_index;
    int black_king_index;
    int white_king_square;
    int black_king_square;
    float time;
};

struct UndoInfo {
    uint8_t castling_rights;
    int white_king_square;
    int black_king_square;
    int halfmove_clock;
    int en_passant_square;
    int captured_piece_index;
    uint64_t zobrist_hash_3fold;
};

struct EngineSettings {
    int colour;
    int depth;
};

struct EngineStats {
    uint64_t nodes;
    int qnodes;
    int checkEvaluations;
    int abPrunes;
    int qcheckmate;
    int checkmate;
    int stalemate;
    int enpassant;
    int castle;
    int capture_cutoff;
    int quiet_cutoff;
    int standpat_cutoff;
    int cutoffs;
    int cutoffstally[100];
};

struct RootMove {
    uint32_t Move;
    int eval;
};

struct EvalBoards {
    int KnightBoard[8][8];
    int BishopBoard[8][8];
    int PawnBoard[8][8];
    int RookBoard[8][8];
    int QueenBoard[8][8];
};

struct pngs {
    Texture2D wPawn;
    Texture2D wKnight;
    Texture2D wBishop;
    Texture2D wRook;
    Texture2D wQueen;
    Texture2D wKing;

    Texture2D bPawn;
    Texture2D bKnight;
    Texture2D bBishop;
    Texture2D bRook;
    Texture2D bQueen;
    Texture2D bKing;
};

struct sounds {
    Sound moveMP3;
    Sound selectMP3;
    Sound captureMP3;
};

struct Pieces {
    int whitePawns;
    int blackPawns;
    int whiteKnights;
    int blackKnights;
    int whiteBishops;
    int blackBishops;
    int whiteRooks;
    int blackRooks;
    int whiteQueens;
    int blackQueens;
};


typedef struct {
    Color boardLight;
    Color boardDark;
    Color panelBg;
    Color panelBorder;
    Color textMuted;
    Color accentCheck;
    Color WINDOW;
    Color highlight;
} ChessTheme;

struct Gui {
    int squareSize;
    int xoffset;
    int yoffset;
    int evalbarSize;
    int evalbarY;
    int textboxSize;
    int boardSize;
    int x;
    int y;
};

struct GuiMove {
    int start_rank;
    int start_file;
    int piece_choosen;
    int target_rank;
    int target_file;
    int promotion_piece;
    int choose_promotion;
};
struct TournamentSettings {
    int total_games;
    int round;
    int time_control;
    char engine1[10];
    char engine2[10];
};

struct PlayingBot {
    char name[20];
    int colour;
    double time_taken;
};


struct TournamentStats {
    int draw;
    int draw_repetition;
    int draw_turnlimit;
    int draw_stalemate;
    int draw_halfmove_clock;
    int draw_illegal_move;
    int engine1_wins;
    int engine1_white_wins;
    int engine1_black_wins;
    int engine2_wins;
    int engine2_white_wins;
    int engine2_black_wins;
};

#endif

