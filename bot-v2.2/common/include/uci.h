#ifndef UCI_H
#define UCI_H
int move_to_uci_string(uint32_t move_to_translate, char out_str[6]);
uint32_t uci_string_to_move(const char in_str[6]);

void write_position(char* fen, const struct GameState* gs, char position_string[8192]);


void pipe_move(uint32_t Move, int cte[2]);
void write_bestmove(uint32_t Move);
void write_info(int evaluation, int currentdepth, int nodes, float time, int pvLength, uint32_t pvTable[64][64]);


 
#endif
