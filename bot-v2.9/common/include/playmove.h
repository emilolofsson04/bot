#ifndef PLAYMOVE_H
#define PLAYMOVE_H


void play_move(Move played_move, struct GameState* gs, struct UndoInfo* ui);
void make_move(Move played_move, struct GameState* gs, struct UndoInfo* ui, struct Evalboards* eb);


#endif

