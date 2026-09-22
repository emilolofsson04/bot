#ifndef UCI_H
#define UCI_H
int move_to_uci_string(Move move_to_translate, char out_str[6]);
Move uci_string_to_move(const char in_str[6]);

void write_bestmove(Move Move);
void write_info(int evaluation, int currentdepth, int seldepth, int nodes, float time, int pvLength, Move pvTable[64][64]);


void stop_and_wait(void);
void parse_position(char* fen, struct GameState* Game);
void parse_go(char* command, struct GameState *Game);
void parse_perft(char* command, struct GameState Game);
void print_board_state(struct GameState Game);
void parse_bench(char* command);


extern int searching;


 
#endif
