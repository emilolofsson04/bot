#include "structs.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <poll.h>
#include <unistd.h>
#include <stdlib.h>



void connectengine(const char* engine_path, int game_to_engine[2], int engine_to_game[2]) {
    pipe(game_to_engine);
    pipe(engine_to_game);



    pid_t pid = fork();
    
 
    if (pid == 0) {
        dup2(game_to_engine[0], STDIN_FILENO);
        dup2(engine_to_game[1], STDOUT_FILENO);
        
        execl(engine_path, engine_path, NULL);
        exit(1);
    }
    close(game_to_engine[0]);
    close(engine_to_game[1]);
}




int findenginemove(char buffer[1000], char move_str[6]) {

    char* move_ptr = strstr(buffer, "bestmove ");
    if (move_ptr != NULL) {
        if (sscanf(move_ptr + 9, "%5s", move_str) == 1) {
            return 1; 
        }
    }
   
    return 0;
}

int findengineeval(char buffer[100], int* eval) {

    char* score_ptr = strstr(buffer, "score cp ");

    if (score_ptr != NULL) {
        if (sscanf(score_ptr + 9, "%d", eval) == 1) {
            return 1; 
        }
    }
    return 0;
}



int read_engine(int engine_to_game[2], char move_str[6], int* eval) {

    struct pollfd pfd;
    pfd.fd = engine_to_game[0];
    pfd.events = POLLIN;

    int ready = poll(&pfd, 1, 0);

    // if it has, read and parse
    if (ready > 0) {
        // read
         
        char buffer[1000];
        int bytes_read = read(engine_to_game[0], buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
                
            buffer[bytes_read] = '\0';
    
            int byte = 0;
            int start_byte = 0;

            while (buffer[byte] != '\0') { // read entire
                if (buffer[byte] == '\n') { //but one message at a time
                
                    buffer[byte] = '\0';
                    char* message = &buffer[start_byte];
                    printf("bot: %s\n", message);


                    findengineeval(message, eval);
                  
                    if (findenginemove(message, move_str)) {
                        return 1;
                    }

                    start_byte = byte + 1;
                }
                byte++;
            }
        }

    }
    return 0;
}
