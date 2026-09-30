#include <stdint.h>
#include <stdlib.h>
#include "zobrist.h"

uint64_t zobrist_pieces[12][64];
uint64_t zobrist_castling_rights[16];
uint64_t zobrist_side_to_move;
uint64_t zobrist_en_passant_file[8];

uint64_t rand_uint64(void) {
    uint64_t r = 0;
        for (int i = 0; i < 64; i++) {
            r = r*2 + rand()%2; 
         
        }
        return r;
}


void init_zobrist() {
    // Pieces and squares
    for (int piece = 0; piece < 12; piece++) {
        for (int square = 0; square < 64; square++) {
            zobrist_pieces[piece][square] = rand_uint64();
        }
    }
    // Side to move 
    zobrist_side_to_move = rand_uint64();

    // Castling Rights (0 to 15)
    for (int i = 0; i < 16; i++) {
        zobrist_castling_rights[i] = rand_uint64();
    }

    // En Passant Files (0 to 7)
    for (int i = 0; i < 8; i++) {
        zobrist_en_passant_file[i] = rand_uint64();
    }
}

