#ifndef UNPLAYMOVE_H
#define UNPLAYMOVE_H






void unplay_move(Move played_move, struct GameState* Game, struct UndoInfo *ui);
void unmake_move(Move played_move, struct GameState* Game, struct UndoInfo *ui);



#endif


