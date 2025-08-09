#ifndef DATACRUMBS_LIBRARY_H
#define DATACRUMBS_LIBRARY_H

// Expose datacrumbs_start() with default visibility for shared libraries.
// Starts the datacrumbs client.
extern "C" __attribute__((visibility("default"))) void datacrumbs_start();

// Expose datacrumbs_stop() with default visibility for shared libraries.
// Stops the datacrumbs client.
extern "C" __attribute__((visibility("default"))) void datacrumbs_stop();

// Function called automatically when the shared library is loaded.
// Used for initialization.
extern void __attribute__((constructor)) datacrumbs_init(void);

// Function called automatically when the shared library is unloaded.
// Used for cleanup.
extern void __attribute__((destructor)) datacrumbs_fini(void);

#endif  // DATACRUMBS_LIBRARY_H