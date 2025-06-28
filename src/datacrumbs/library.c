
#include "library.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
__attribute__((visibility("default"))) void datacrumbs_start() {}
__attribute__((visibility("default"))) void datacrumbs_stop() {}

void datacrumbs_init(void) {
    datacrumbs_start();
    char *env = getenv("DATACRUMBS_USDT_ENABLE");
    if (env == NULL || strcmp(env, "1") != 0) {
        return;
    }
    sleep(15);
}

void datacrumbs_fini(void) { datacrumbs_stop(); }