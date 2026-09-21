#include <stdio.h>
#include <string.h>
#include "types.h"
#include "uci.h"
#include "tt.h"
#include "zobrist.h"
#include "params.h"



int main() {

    setbuf(stdout, NULL);

    char line[INPUT_BUFFER_SIZE];

    struct GameState Game;

    init_tt();
    init_zobrist();

    while (fgets(line, INPUT_BUFFER_SIZE, stdin) != NULL) {

        line[strcspn(line, "\n")] = 0;

        if (strlen(line) == 0) continue;


        if (strcmp(line, "uci") == 0) {
            printf("id name bot\n");
            printf("id author Emil\n");
            printf("uciok\n");
        }
        else if (strcmp(line, "isready") == 0) {
            printf("readyok\n");
        }
        else if (strcmp(line, "ucinewgame") == 0) {
            stop_and_wait();
            reset_tt();
            Game = (struct GameState){0};
 
        }
        else if (strncmp(line, "position ", 9) == 0) {
            stop_and_wait();
            parse_position(line, &Game);
        }
        else if (strncmp(line, "go", 2) == 0) {
            stop_and_wait();

            searching = 1;
            tt_generation++;
            parse_go(line, &Game);
        }
        else if (strncmp(line, "perft", 5) == 0) {
            parse_perft(line, Game);
        }
        else if (strcmp(line, "quit") == 0) {
            stop_and_wait();
            break;
        }
        else if (strcmp(line, "stop") == 0) {
            stop_and_wait();
        }
        else
            printf("Unknown command: '%s'. \n", line);
    }

    free_tt();

    return 0;
}
