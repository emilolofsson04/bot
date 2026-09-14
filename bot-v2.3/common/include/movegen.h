#ifndef MOVEGEN_H
#define MOVEGEN_H





void find_semi_moves(int* total_semi_quiet_moves, int* total_semi_captures, uint32_t legal_moves[256], uint32_t legalCaptures[256], struct GameState* gs);
int generate_legal_moves(struct GameState* gs, uint32_t legal_moves[256]);



int is_move_legal(struct GameState* gs, char input[6], uint32_t* matched_legal_move);

int is_king_safe(int board[8][8], int rank, int file, int pcolour);


int generate_legal_captures(struct GameState* gs, uint32_t legal_moves[256]);








#endif
