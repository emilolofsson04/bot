#ifndef BOARD_H 
#define BOARD_H


void print_board(int board[8][8]);
void update_board(struct GameState* gs);
//void read_fen(char fen_string[100], struct GameState* gs);
void read_fen(const char *fen_string, struct GameState* gs);
int write_fen(const struct GameState* gs, char* fen);






#endif
