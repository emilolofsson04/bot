#ifndef SEARCH_H
#define SEARCH_H

#pragma once
#include <stdatomic.h>
#include <stdbool.h>

extern atomic_bool stop_search;

int negamax(struct GameState* Game, struct SearchContext* Search, struct NodeState Node, int alpha, int beta);
Move search_start(struct GameState Game, int max_depth, int time_left, int increment_time, int move_time);

#endif
