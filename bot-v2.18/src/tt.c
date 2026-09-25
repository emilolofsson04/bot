#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tt.h"
#include "params.h"

struct tt_entry *TT = NULL;
int tt_generation = 0;

void init_tt(void) {
    TT = calloc(TT_SIZE, sizeof(*TT));

     if (TT == NULL) {
        fprintf(stderr, "Failed to allocate transposition table\n");
        exit(EXIT_FAILURE);
    }
}

void reset_tt(void) {
    memset(TT, 0, TT_SIZE * sizeof(*TT));
}

void free_tt(void) {
    free(TT);
    TT = NULL;
}
