#ifndef BOARD_H 
#define BOARD_H


void print_board(int board[8][8]);
void update_board(struct GameState* Game);
//void read_fen(char fen_string[100], struct GameState* Game);
void read_fen(const char *fen_string, struct GameState* Game);
int write_fen(const struct GameState* Game, char* fen);






#endif
