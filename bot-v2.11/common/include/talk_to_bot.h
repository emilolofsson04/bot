#ifndef TALKTOBOT_H
#define TALKTOBOT_H
int read_engine(int engine_to_game[2], char move_str[6], int* eval);

void connectengine(const char* engine_path, int game_to_engine[2], int engine_to_game[2]);
#endif


