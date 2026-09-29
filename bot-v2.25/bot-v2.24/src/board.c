#include <stdio.h>
#include <string.h>


#include "types.h"
#include <stdlib.h>

#include <inttypes.h>
#include "zobrist.h"

void print_board(int board[8][8]) {
    char white_pieces[7] = {' ', 'P', 'N', 'B', 'R', 'Q', 'K'};  
    char black_pieces[7] = {' ', 'p', 'n', 'b', 'r', 'q', 'k'};  
    printf("\n");    
    printf(" +---+---+---+---+---+---+---+---+\n");
    for(int i = 7; i >= 0; i--) {
        printf(" ");
        for(int k = 0; k < 8; k++) {
            printf("| ");
            if (board[i][k] >= 0) {
                printf("%c", white_pieces[board[i][k]]);
            }
            else {
                printf("%c", black_pieces[abs(board[i][k])]);
            }
            printf(" ");
        }
        printf("|\n");
        printf(" +---+---+---+---+---+---+---+---+\n");
    }
    printf("\n");    
}

void print_bit_board(uint64_t board) {
    printf("\n");
    printf("   +---+---+---+---+---+---+---+---+\n");
    for (int r = 7; r >= 0; r--) {
        printf(" %d |", r + 1); // Rank labels 8 down to 1
        for (int f = 0; f < 8; f++) {
            int sq = r * 8 + f;
            int bit = (board & (1ULL << sq)) ? 1 : 0;

            if (bit) {
                printf(" X |"); // 'X' or '1' makes active bits pop out
            } else {
                printf(" . |");
            }
        }
        printf("\n   +---+---+---+---+---+---+---+---+\n");
    }
    printf("     a   b   c   d   e   f   g   h\n\n");
    printf(" Hex: 0x%016" PRIx64 "\n\n", board);
}
void update_board(struct GameState* Game) {

    /*
     Makes the board based on the piece structs 
     */
    
    int rank;
    int file;
    for(int i = 0; i < 8; i++) {
        for(int k = 0; k < 8; k++) {
            Game->board[i][k] = 0;
            Game->Index_board[i][k] = -1;

        }
    }
    for(int i = 0; i < 16; i++) {
        if (Game->white_pieces[i].alive) {
            rank = Game->white_pieces[i].rank;
            file = Game->white_pieces[i].file;
        
            Game->board[rank][file] = Game->white_pieces[i].type;
            Game->Index_board[rank][file] = i;

        }
    }
    for(int i = 0; i < 16; i++) {
        if (Game->black_pieces[i].alive) {
            rank = Game->black_pieces[i].rank;
            file = Game->black_pieces[i].file;
            Game->board[rank][file] = Game->black_pieces[i].type;
            Game->Index_board[rank][file] = i;
        }
    }
}




int piece_value(char c) {
    switch (c)   {
        case 'P': return  1;
        case 'N': return  2;
        case 'B': return  3;
        case 'R': return  4;
        case 'Q': return  5;
        case 'K': return  6;

        case 'p': return -1;
        case 'n': return -2;
        case 'b': return -3;
        case 'r': return -4;
        case 'q': return -5;
        case 'k': return -6;

        default: return 0;
    }
}

void read_fen(const char *fen_string, struct GameState* Game) {

    Game->ply = 0;

    // Reset caslting rights and deny en passant unless given in fen
    Game->castling_rights = 0;
    Game->en_passant_square = -1;
    Game->zobrist_hash = 0;


    // Kill all pieces to ensure to old memory causes issues
    for (int i = 0; i < 16; i++) {
        Game->white_pieces[i].alive = 0;
        Game->black_pieces[i].alive = 0;
    }
    

    int total_white_pieces = 0;
    int total_black_pieces = 0;
    int fen_length = strlen(fen_string);
    int i = 0;

    int rank = 7;
    int file = 0;
    int pieceValues[7] = {0,1,3,3,5,9,50};

    // First part of fen (board piece positions)
    while (i < fen_length && fen_string[i] != ' ') {
        char c = fen_string[i];

        if (c == '/') {
            rank--;
            file = 0;
        }
        else if (c >= '1' && c <= '8') {
            file += c - '0'; 
        }
        else {
            int piece = piece_value(c); 


            struct piece* Piece;
            if (piece > 0) {
                Piece = &Game->white_pieces[total_white_pieces];
                if (piece == 6) {
                    Game->white_king_index = total_white_pieces;
                    Game->white_king_square = 8*rank + file;
                }
                Piece->colour = 1;
                total_white_pieces++;
                Game->zobrist_hash ^= zobrist_pieces[piece - 1][rank * 8 + file];
            }
            else {
                Piece = &Game->black_pieces[total_black_pieces];
                if (piece == -6) {
                    Game->black_king_index = total_black_pieces;
                    Game->black_king_square = 8*rank + file;
                }
                Piece->colour = 0;
                total_black_pieces++;
                Game->zobrist_hash ^= zobrist_pieces[6 + abs(piece) - 1][rank * 8 + file];
            }
            Piece->value = pieceValues[abs(piece)];
            Piece->rank = rank;
            Piece->file = file;
            Piece->type = piece;
            Piece->alive = 1;

            file++;
        }
        i++;
    }
    i++; // skip ' '

    // Side to move is next (w or b)
    Game->side_to_move = (fen_string[i] == 'w') ? SIDE_WHITE : SIDE_BLACK;
    if (Game->side_to_move == SIDE_BLACK) {
        Game->zobrist_hash ^= zobrist_side_to_move;
    }

    i += 2; 

    // Castling rights
    while (i < fen_length && fen_string[i] != ' ') {
        switch (fen_string[i]) {
            case 'K': Game->castling_rights += WK; break;
            case 'Q': Game->castling_rights += WQ; break;
            case 'k': Game->castling_rights += BK; break;
            case 'q': Game->castling_rights += BQ; break;
            case '-': break; 
        }
        i++;
    }
    Game->zobrist_hash ^= zobrist_castling_rights[Game->castling_rights];

    i++; // Skip the space

    // en passant square
    if (fen_string[i] != '-') {
        int ep_file = fen_string[i] - 'a';
        int ep_rank = fen_string[i+1] - '1'; 

        Game->en_passant_square = (ep_rank * 8) + ep_file;
        Game->zobrist_hash ^= zobrist_en_passant_file[ep_file];
        i += 2;  
    } 
    else {
        Game->en_passant_square = -1;
        i++; 
    }
    i++;
    Game->halfmove_clock = fen_string[i] - '0';

    //Finaly make the board
    update_board(Game);

    Game->zobrist_hash_3fold_history[Game->ply] = Game->zobrist_hash;

}
void set_up_startpos(struct GameState* Game) {
    char start_fen[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 \0";
    read_fen(start_fen, Game);
}

char piece_int_to_char(int p) {
    switch (p) {
        case  1: return 'P';
        case  2: return 'N';
        case  3: return 'B';
        case  4: return 'R';
        case  5: return 'Q';
        case  6: return 'K';

        case -1: return 'p';
        case -2: return 'n';
        case -3: return 'b';
        case -4: return 'r';
        case -5: return 'q';
        case -6: return 'k';

        default: return '.';
    }
}


int write_fen(const struct GameState* Game, char* fen) {


    int rank = 7;
    int file = 0;
    int i = 0;

    for (int r = rank; r >= 0; r--) {
        int empty_files = 0;
        for (int f = file; f < 8; f++) {
            
            int square = Game->board[r][f];
            if (square != EMPTY) {
                if (empty_files == 0) {
                    char piece = piece_int_to_char(square);
                    fen[i] = piece;
                    i++;
                }
                else {
                    fen[i] = empty_files + '0';
                    i++;
                    char piece = piece_int_to_char(square);
                    fen[i] = piece;
                    empty_files = 0;
                    i++;
                }
            }
            else {
                empty_files++;
            }

        }
        if (empty_files != 0) {
            sprintf(&fen[i], "%d", empty_files);
            i++;
        }
        fen[i] = '/';
        i++;
    }


    fen[i - 1] = ' '; // Deletes the last / and replaces wiht a space;

    fen[i]      = (Game->side_to_move == SIDE_WHITE) ? 'w' : 'b';
    fen[i + 1]  = ' ';
    i += 2;


    if (Game->castling_rights & WK) {
        fen[i] = 'K';
        i++;
    }
    if (Game->castling_rights & WQ) {
        fen[i] = 'Q';
        i++;
    }
    if (Game->castling_rights & BK) {
        fen[i] = 'k';
        i++;
    }
    if (Game->castling_rights & BQ) {
        fen[i] = 'q';
        i++;
    }
    if (Game->castling_rights == 0) {
        fen[i] = '-';
        i++;
    }

    fen[i] = ' ';
    i++;

    if (Game->en_passant_square != -1) {
        fen[i]  = Game->en_passant_square % 8 + 'a';
        fen[i + 1]      = Game->en_passant_square / 8 + '1';
        i += 2;
    }
    else {
        fen[i] = '-'; 
        i++;
    }


    fen[i] = '\n';
    fen[i + 1] = '\0';
    
    return i;

}

