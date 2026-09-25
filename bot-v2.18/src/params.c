#include "params.h"
#include "types.h"
#include <stdio.h>
#include <string.h>
#include "search.h"

struct SearchParams Params;

struct OptionDef {
    const char* name;
    int* target_val;
    int default_val;
    int min_val;
    int max_val;
};

static const struct OptionDef OPTIONS_TABLE[] = {
    {"AspirationDelta",    &Params.aspiration_delta,          DEFAULT_ASPIRATION_DELTA,    5, 200},
    {"AspirationMinDepth", &Params.aspiration_min_depth,  DEFAULT_ASPIRATION_MIN_DEPTH,    1,  10},
    {"NMPBaseReduction",   &Params.nmp_base_reduction,      DEFAULT_NMP_BASE_REDUCTION,    1,   5},
    {"NMPMinDepth",        &Params.nmp_min_depth,                DEFAULT_NMP_MIN_DEPTH,    1,  10},
    {"NMPDepthReduction",  &Params.nmp_depth_divisor,        DEFAULT_NMP_DEPTH_DIVISOR,    1,  10},
};

int NUM_OPTIONS = sizeof(OPTIONS_TABLE) / sizeof(OPTIONS_TABLE[0]);

void init_default_params(void) {
    for (int option = 0; option < NUM_OPTIONS; option++) {
        *(OPTIONS_TABLE[option].target_val) = OPTIONS_TABLE[option].default_val;
    }
    init_lmr_table();
}

void print_uci_options(void) {
    for (int i = 0; i < NUM_OPTIONS; i++) {
        printf("option name %s type spin default %d min %d max %d\n",
               OPTIONS_TABLE[i].name,
               OPTIONS_TABLE[i].default_val,
               OPTIONS_TABLE[i].min_val,
               OPTIONS_TABLE[i].max_val);
    }
}


void set_option(const char* line) {
    char opt_name[64];
    int opt_value;

    if (sscanf(line, "setoption name %63s value %d", opt_name, &opt_value) == 2) {
        for (int i = 0; i < NUM_OPTIONS; i++) {
            if (strcmp(opt_name, OPTIONS_TABLE[i].name) == 0) {

                if (opt_value < OPTIONS_TABLE[i].min_val) opt_value = OPTIONS_TABLE[i].min_val;
                if (opt_value > OPTIONS_TABLE[i].max_val) opt_value = OPTIONS_TABLE[i].max_val;

                *(OPTIONS_TABLE[i].target_val) = opt_value;

                return;
            }
        }
    }
}
