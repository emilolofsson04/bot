#include <stdlib.h>
#include "structs.h"
#include <stdio.h>


#include "movegen.h"
#include "playmove.h"
#include "unplaymove.h"
#include "uci.h"

#include <assert.h>


#include <assert.h>

void verify_state_restored(const struct GameState* original, const struct GameState* current) {

    // Check board matches
    for (int i = 0; i < 8; i++) {
        for (int k = 0; k < 8; k++) {
            assert(original->board[i][k] == current->board[i][k]);
        }
    }

    // Check board matches
    for (int i = 0; i < 8; i++) {
        for (int k = 0; k < 8; k++) {
            assert(original->Index_board[i][k] == current->Index_board[i][k]);
        }
    }



    // Verify white pieces match
    for (int i = 0; i < 16; i++) {
        assert(original->white_pieces[i].rank == current->white_pieces[i].rank);
        assert(original->white_pieces[i].file == current->white_pieces[i].file);
        assert(original->white_pieces[i].alive == current->white_pieces[i].alive);
        assert(original->white_pieces[i].value == current->white_pieces[i].value);
        assert(original->white_pieces[i].type == current->white_pieces[i].type);
    }

    // Verify black pieces matches
    for (int i = 0; i < 16; i++) {
        assert(original->black_pieces[i].rank == current->black_pieces[i].rank);
        assert(original->black_pieces[i].file == current->black_pieces[i].file);
        assert(original->black_pieces[i].alive == current->black_pieces[i].alive);
        assert(original->black_pieces[i].value == current->black_pieces[i].value);
        assert(original->black_pieces[i].type == current->black_pieces[i].type);
    }

    // Side to move and caslting rights
    assert(original->side_to_move == current->side_to_move);
    assert(original->castling_rights == current->castling_rights);
    assert(original->zobrist_hash == current->zobrist_hash);
}


static void PerftBranch(struct GameState* gs, int depth, struct EngineStats* es) {

    /*
     Perft test branch.
    */


    int total_legal_moves = 0;
    
    Move legal_moves[256];



    // Count node
    if (depth == 0) {
        (es->nodes)++;
        return;
    }

    total_legal_moves = generate_legal_moves(gs, legal_moves);

    if (depth  == 1){
        es->nodes += total_legal_moves;
        return;
    }

    for (int k = 0; k < total_legal_moves; k++) {
        struct UndoInfo ui;

        play_move(legal_moves[k], gs, &ui); 

        PerftBranch(gs, depth - 1, es);

        unplay_move(legal_moves[k], gs, &ui);
    }


    return;
}


void Perft(struct GameState gs, int depth, struct EngineStats* es) {

     /*
      Performs a perft.
      Tests speed  and correctness
      of movegen, play_move and unplay_move.
    */


    // Find all legal moves 
    Move legal_moves[256];
    int total = generate_legal_moves(&gs, legal_moves);

    struct GameState gs_copy = gs;
 
    uint64_t old_nodes = 0;

    // Iterate over legalmoves
    for (int k = 0; k < total; k++) {

        // Play move 
        struct UndoInfo ui;
        play_move(legal_moves[k], &gs, &ui);

        // Spaw color and call branch function
        PerftBranch(&gs, depth - 1, es);


        // Unplay the move
        unplay_move(legal_moves[k], &gs, &ui);

        char move_str[6];
        move_to_uci_string(legal_moves[k], move_str);
        printf("%s: %lu\n", move_str, es->nodes - old_nodes);
        old_nodes = es->nodes;
    }

    verify_state_restored(&gs_copy, &gs);
}

