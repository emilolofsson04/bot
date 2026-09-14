#ifndef UNPLAYMOVE_H
#define UNPLAYMOVE_H






void unplay_move(Move played_move, struct GameState* gs, struct UndoInfo *ui);
void unmake_move(Move played_move, struct GameState* gs, struct UndoInfo *ui);



#endif


