#ifndef HELPER_DEFINITIONS_H
#define HELPER_DEFINITIONS_H
#include <stdlib.h>

#define ABS(X) ((X) < 0 ? -(X) : (X))

#define mem_check(POINTER) {\
    if (( POINTER ) == NULL) {\
        fprintf(stderr, "ERROR: Failed to allocate memory.\n");\
        exit(EXIT_FAILURE); \
    }\
}

// Quick double-precision random number [0, 1). From K&R.
#define frand() ((double) rand() / (RAND_MAX+1.0))
// crand, circular-rand: random number in [0, 2PI)
#define crand() ( frand() * 2 * M_PI )

#endif // HELPER_DEFINITIONS_H
