#ifndef UCI_H
#define UCI_H
int move_to_uci_string(Move move_to_translate, char out_str[6]);
Move uci_string_to_move(const char in_str[6]);

void write_position(char* fen, const struct GameState* gs, char position_string[8192]);


void pipe_move(Move Move, int cte[2]);
void write_bestmove(Move Move);
void write_info(int evaluation, int currentdepth, int nodes, float time, int pvLength, Move pvTable[64][64]);


 
#endif
