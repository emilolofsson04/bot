#include <stdio.h>
#include "structs.h"
#include <string.h>
#include "movegen.h"
#include "playmove.h"
#include "search.h"
#include <string.h>
#include <stdlib.h>
#include "perft.h"
#include <time.h>
#include "board.h"
#include "uci.h"
#include "tt.h"
#include "zobrist.h"




void parse_position(char* fen, struct GameState* gs) {

    char* fen_str_ptr        = strstr(fen, "fen");
    char* move_str_ptr           = strstr(fen, "moves");
    char* start_position_ptr = strstr(fen, "startpos");

    // If given fen, set up gamestate based on it
    if (fen_str_ptr != NULL && start_position_ptr == NULL) {

        if (move_str_ptr != NULL) {
            char* end_fen_str = move_str_ptr - 1;
            *end_fen_str = '\0';
        }

        read_fen((fen_str_ptr + 4), gs);
    }

    // If we are given startpos, initiate gamestate from start position fen
    if (start_position_ptr != NULL) {
        char start_fen[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 \0";
        read_fen(start_fen, gs);
    }


    // If we recieve moves, parse and play them
    if (move_str_ptr != NULL) {

        if (strlen(move_str_ptr) > 6) {
            
            char* move = strtok(move_str_ptr + 6, " ");

            while (move != NULL) {

                uint32_t legal_move;
                if (is_move_legal(gs, move, &legal_move)) {
                    struct UndoInfo ui;
                    play_move(legal_move, gs, &ui);

                    move = strtok(NULL, " ");
                }
                else {
                    printf("Illegal move in position: %s\n", move);
                    fflush(stdout);
                    break;
                }
            }
        }
    }
}

void parse_go(char* command, struct GameState *gs) {

    int depth =  5;
    int time = -1;
    char* depth_ptr = strstr(command, "depth");

    if (depth_ptr != NULL) {
        sscanf(depth_ptr, "depth %d", &depth);
    }
    
    char* time_ptr = strstr(command, "time");

    if (time_ptr != NULL) {
        sscanf(time_ptr, "time %d", &time);
        time = time / 30; // Use 1/30 the time remaining
    }


    uint32_t BestMove;
    BestMove = iterative_deepening(*gs, depth, time);
    write_bestmove(BestMove);
}


void parse_perft(char* command, struct GameState gs) {

    // Set and find depth to perft
    int depth;

    char* depth_ptr = strstr(command, "depth");

    if (depth_ptr != NULL) {
        if (sscanf(depth_ptr, "depth %d", &depth) != 1) {
            depth = 5;
        }
    }
    else {
        return;
    }
    
    // Stats
    struct EngineStats perfstats = {0};

    // Time perft 
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    Perft(gs, depth, &perfstats); // Run perft

    // End time
    clock_gettime(CLOCK_MONOTONIC, &end); 
    double time_taken =
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Nodes searched: %ld\n", perfstats.nodes);
    printf("Time: %.3f s\n", time_taken);
    printf("NPS: %.0f\n", (float)perfstats.nodes / time_taken);
}


int main() {

    int INPUT_BUFFER_SIZE = 8192;
    char line[INPUT_BUFFER_SIZE];

    struct GameState gs;
    TT = (struct tt_entry*)calloc(TT_SIZE, sizeof(struct tt_entry));
    init_zobrist();
    printf("TT_SIZE %d\n", TT_SIZE * sizeof(struct tt_entry));



    while (fgets(line, INPUT_BUFFER_SIZE, stdin) != NULL) {

        // remove new line
        line[strcspn(line, "\n")] = 0;

        if (strlen(line) == 0) continue;


        if (strcmp(line, "uci") == 0) {
            printf("id name bot-v2\n");
            printf("id author Emil\n");
            printf("uciok\n");
        }
        else if (strcmp(line, "isready") == 0) {
            printf("readyok\n");
        }
        else if (strcmp(line, "ucinewgame") == 0) {
            memset(TT, 0, TT_SIZE * sizeof(*TT));
            gs = (struct GameState){0};
 
        }
        else if (strncmp(line, "position", 8) == 0) {
            parse_position(line, &gs);
        }
        else if (strncmp(line, "go", 2) == 0) {
            tt_generation++;
            printf("tt gen %d  \n", tt_generation);
            parse_go(line, &gs);
        }
        else if (strncmp(line, "perft", 5) == 0) {
            parse_perft(line, gs);
        }
        else if (strcmp(line, "quit") == 0) {
            break;
        }
        else if (strcmp(line, "stop") == 0) {
            // Stop engine eventually  
        }

    }

    free(TT);

    return 0;
}
