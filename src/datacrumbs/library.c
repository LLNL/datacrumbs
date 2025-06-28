
#include "library.h"
#include <unistd.h>
__attribute__((visibility("default"))) void datacrumbs_start() {}
__attribute__((visibility("default"))) void datacrumbs_stop() {}

void datacrumbs_init(void) {
    datacrumbs_start();
    sleep(15);
}

void datacrumbs_fini(void) { datacrumbs_stop(); }