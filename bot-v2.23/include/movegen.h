#ifndef MOVEGEN_H
#define MOVEGEN_H





void find_semi_moves(int* total_semi_quiet_moves, int* total_semi_captures, Move legal_moves[256], Move legalCaptures[256], struct GameState* Game);
int generate_legal_moves(struct GameState* Game, Move legal_moves[256]);



int is_move_legal(struct GameState* Game, char input[6], Move* matched_legal_move);

int is_king_safe(int board[8][8], int rank, int file, int pcolour);


int generate_legal_captures(struct GameState* Game, Move legal_moves[256]);








#endif
