#ifndef SEARCH_H
#define SEARCH_H

static int negamax(struct GameState* Game, struct SearchContext* Search, struct NodeState Node, int alpha, int beta);



Move iterative_deepening(struct GameState gs, int max_depth, int max_time);

#endif
