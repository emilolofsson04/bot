#include "structs.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <poll.h>
#include <unistd.h>
#include <stdlib.h>




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

void pipe_move(uint32_t Move, int cte[2]) {

    /* Pipes move to bot */

    char inputmove[6] = {0};

    move_to_uci_string(Move, inputmove);

    // Recieved by fgets which expects a new line
    char pipe_buffer[10];
    sprintf(pipe_buffer, "%s\n", inputmove);

    int bytes_sent = write(cte[1], pipe_buffer, strlen(pipe_buffer));
    printf("Sent %d bytes to bot: %s", bytes_sent, pipe_buffer);
}

void write_bestmove(uint32_t Move) {

    /* Writes bestmove from bot */

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



