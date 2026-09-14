#include <stdlib.h>
#include "tt.h"

struct tt_entry *TT = NULL;
int TT_SIZE = 1 << 20;

void tt_init(void) {
    TT = calloc(TT_SIZE, sizeof(*TT));
}

void tt_free(void) {
    free(TT);
    TT = NULL;
}
