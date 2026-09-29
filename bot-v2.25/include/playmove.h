#ifndef PLAYMOVE_H
#define PLAYMOVE_H


void play_move(Move played_move, struct GameState* Game, struct UndoInfo* ui);
void make_move(Move played_move, struct GameState* Game, struct UndoInfo* ui);


#endif

