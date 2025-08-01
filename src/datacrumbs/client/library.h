#ifndef DATACRUMBS_LIBRARY_H
#define DATACRUMBS_LIBRARY_H
__attribute__((visibility("default"))) extern void datacrumbs_start();

__attribute__((visibility("default"))) extern void datacrumbs_stop();

extern void __attribute__((constructor)) datacrumbs_init(void);

extern void __attribute__((destructor)) datacrumbs_fini(void);

#endif // DATACRUMBS_LIBRARY_H