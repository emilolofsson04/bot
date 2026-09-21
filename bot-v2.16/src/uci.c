#include "types.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <poll.h>
#include <unistd.h>
#include <stdlib.h>
#include "search.h"
#include "perft.h"
#include <pthread.h>
#include <time.h>
#include "board.h"
#include "movegen.h"
#include "playmove.h"
#include "uci.h"



pthread_t search_thread;
int searching = 0;

int move_to_uci_string(uint32_t move_to_translate, char out_str[6]) {

    /*
     Converts the move struct into a uci string
     */

    int start_square = get_from_square(move_to_translate);
    int to_square = get_to_square(move_to_translate);
   if (get_move_type(move_to_translate) == PROMOTION) {
       char Promo[4] = {'q', 'r', 'b', 'n'};
        char promo_char = Promo[get_promotion_type(move_to_translate)];

        sprintf(out_str, "%c%c%c%c%c",
            start_square % 8 + 'a', start_square / 8 + '1',
            to_square % 8 + 'a', to_square / 8 + '1',
            promo_char);
   }
   else if (get_move_type(move_to_translate) == SHORT_CASTLE) {

        sprintf(out_str, "%c%c%c%c",
            start_square % 8 + 'a', start_square / 8 + '1',
            to_square % 8 + 'a' - 1, to_square / 8 + '1');
   }
   else if (get_move_type(move_to_translate) == LONG_CASTLE) {

        sprintf(out_str, "%c%c%c%c",
            start_square % 8 + 'a', start_square / 8 + '1',
            to_square % 8 + 'a' + 2, to_square / 8 + '1');
   }
   else {

        sprintf(out_str, "%c%c%c%c",
            start_square % 8 + 'a', start_square / 8 + '1',
            to_square % 8 + 'a', to_square / 8 + '1');
   }

    return strlen(out_str);
}

uint32_t uci_string_to_move(const char in_str[6]) {

    /*
     Converts the input move string into a move struct
     */

    uint32_t input_move;
    if(strlen(in_str) >= 4) {

        int file = in_str[0]-'a';
        int target_file = in_str[2]-'a';
        int rank = in_str[1]-'1';
        int target_rank = in_str[3]-'1';
        uint32_t move = ((rank * 8 + file) << FROM_SHIFT)
           | ((target_rank * 8 + target_file) << TO_SHIFT);



        input_move = move;

        if (isalpha(in_str[4])) {
           char Promo[4] = {'q', 'r', 'b', 'n'};

           int promo = 0;
            for (int i = 0; i < 4; i++) {
                if (in_str[4] == Promo[i]) {
                    promo = i;
                    break;
                }
            }
            input_move |=  (promo << PROMOTION_TYPE_SHIFT);
        }
    }
    return input_move;
}


void write_bestmove(uint32_t Move) {

    /* Writes bestmove from bot */

    if (Move == 0) return;

    char outputmove[6] = {0};

    move_to_uci_string(Move, outputmove);
    printf("bestmove %s\n",outputmove);

    fflush(stdout);
}

void write_info(int evaluation, int currentdepth, int nodes, float time, int pvLength, uint32_t pvTable[64][64]) {

    /* Writes info from bot */

    char ponder_str[6 * 64]; // Length of the longest move * longest pvTable
    int end_of_str = 0;

    int move_to_print = currentdepth;

    if (abs(evaluation) > (90000)) {
        move_to_print = pvLength;
    }
    for (int i = 0; i < move_to_print; i++) {

        end_of_str += move_to_uci_string(pvTable[0][i], &ponder_str[end_of_str]); // Shift end with length of move string

        // Sprintf null terminates, so we overwrite it except if its the last move
        if (i != pvLength) {
            ponder_str[end_of_str] = ' ';
            end_of_str++;
        }
    }
    ponder_str[end_of_str] = '\0'; // I trust no null termination but my own

    printf("info depth %2d score cp %5d nodes %9d nps %8.f time %5.f pv %s\n", currentdepth, evaluation, nodes, nodes/(time), 1000*time, ponder_str);

    fflush(stdout);
}




void parse_position(char* fen, struct GameState* Game) {

    char* fen_str_ptr        = strstr(fen, "fen");
    char* move_str_ptr           = strstr(fen, "moves");
    char* start_position_ptr = strstr(fen, "startpos");

    // If given fen, set up gamestate based on it
    if (fen_str_ptr != NULL && start_position_ptr == NULL) {

        if (move_str_ptr != NULL) {
            char* end_fen_str = move_str_ptr - 1;
            *end_fen_str = '\0';
        }

        read_fen((fen_str_ptr + 4), Game);
    }

    // If we are given startpos, initiate gamestate from start position fen
    if (start_position_ptr != NULL) {
        char start_fen[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 \0";
        read_fen(start_fen, Game);
    }


    // If we recieve moves, parse and play them
    if (move_str_ptr != NULL) {

        if (strlen(move_str_ptr) > 6) {

            char* move = strtok(move_str_ptr + 6, " ");

            while (move != NULL) {

                Move legal_move;
                if (is_move_legal(Game, move, &legal_move)) {
                    struct UndoInfo ui;
                    play_move(legal_move, Game, &ui);

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

void stop_and_wait(void) {
    if (searching) {
        atomic_store(&stop_search, true);
        pthread_join(search_thread, NULL);
        searching = 0;
    }
}

void *search(void *arg) {

    struct SearchParams *params = arg;

    Move best_move = search_start(params->Game, params->depth, params->time_left, params->increment, params->move_time, params->node_limit);

    write_bestmove(best_move);
    searching = 0;

    return NULL;
}


void parse_go(char* command, struct GameState *Game) {


    struct SearchParams params = {0};
    params.Game = *Game;

    char* depth_ptr = strstr(command, "depth");

    if (depth_ptr != NULL) {
        sscanf(depth_ptr, "depth %d", &params.depth);
    }

    char* move_time_ptr = strstr(command, "movetime");

    if (move_time_ptr != NULL) {
        sscanf(move_time_ptr, "movetime %d", &params.move_time);
    }

    char* node_ptr = strstr(command, "nodes");

    if (node_ptr != NULL) {
        sscanf(node_ptr, "nodes %d", &params.node_limit);
    }


    char* time_ptr;
    char* increment_ptr;

    if (Game->side_to_move == SIDE_WHITE) {
        time_ptr        = strstr(command, "wtime");
        increment_ptr   = strstr(command, "winc");
    }
    else {
        time_ptr        = strstr(command, "btime");
        increment_ptr   = strstr(command, "binc");
    }

    if (time_ptr != NULL) {

        if (Game->side_to_move == SIDE_WHITE)
            sscanf(time_ptr, "wtime %d", &params.time_left);
        else
            sscanf(time_ptr, "btime %d", &params.time_left);
    }

    if (increment_ptr != NULL) {

        if (Game->side_to_move == SIDE_WHITE)
            sscanf(increment_ptr, "winc %d", &params.increment);
        else
            sscanf(increment_ptr, "binc %d", &params.increment);
    }

    atomic_store(&stop_search, false);
    pthread_create(&search_thread, NULL, &search, &params);
}



void parse_perft(char* command, struct GameState Game) {

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


    // Time perft
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    struct EngineStats perfstats = {0};
    Perft(Game, depth, &perfstats);

    // End time
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken =
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Nodes searched: %ld\n", perfstats.nodes);
    printf("Time: %.3f s\n", time_taken);
    printf("NPS: %.0f\n", (float)perfstats.nodes / time_taken);
}



