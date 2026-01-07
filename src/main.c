#include <stdio.h>
#include "engine/engine.h"

int main(void) {
    Engine* engine = engine_init();
    if (!engine) {
        fprintf(stderr, "Failed to initialize engine\n");
        return -1;
    }
    
    engine_run(engine);
    engine_cleanup(engine);
    
    return 0;
}
