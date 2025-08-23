/**
 * @file library.cpp
 * @brief Implementation of library initialization and finalization functions for the datacrumbs
 * client.
 *
 * This file contains functions that are called when the library is loaded and unloaded.
 * These functions are responsible for initializing and finalizing the library's functionality.
 */

/**
 * Standard headers
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * Internal headers
 */
#include <datacrumbs/client/library.h>
#include <datacrumbs/common/logging.h>
#include <datacrumbs/datacrumbs_config.h>

/**
 * @brief Called when the library is loaded.
 *
 * This function logs a message indicating that the library's start function has been called,
 * along with the current process ID.
 */
extern "C" __attribute__((visibility("default"))) void datacrumbs_start() {
  int pid = getpid();
  DC_LOG_INFO("Start called (pid: %d)", pid);
}

/**
 * @brief Called when the library is unloaded.
 *
 * This function logs a message indicating that the library's stop function has been called,
 * along with the current process ID.
 */
extern "C" __attribute__((visibility("default"))) void datacrumbs_stop() {
  int pid = getpid();
  DC_LOG_INFO("Stop called (pid: %d)", pid);
}

/**
 * @brief Library initialization function.
 *
 * This function is intended to be called automatically when the library is loaded.
 * It calls the datacrumbs_start function to perform any necessary startup actions.
 */
void datacrumbs_init(void) {
  datacrumbs_start();
}

/**
 * @brief Library finalization function.
 *
 * This function is intended to be called automatically when the library is unloaded.
 * It calls the datacrumbs_stop function to perform any necessary cleanup actions.
 */
void datacrumbs_fini(void) {
  datacrumbs_stop();
}