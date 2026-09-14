#ifndef SEARCH_H
#define SEARCH_H

static struct SearchResult branch(struct GameState* Game, struct NodeState Node, struct SearchContext* Search);



Move iterative_deepening(struct GameState gs, int max_depth, int max_time);

#endif
