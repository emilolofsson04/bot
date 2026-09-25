#ifndef BOARD_H 
#define BOARD_H


void print_board(int board[8][8]);
void update_board(struct GameState* Game);
void read_fen(const char *fen_string, struct GameState* Game);
void set_up_startpos(struct GameState* Game);
int write_fen(const struct GameState* Game, char* fen);






#endif
