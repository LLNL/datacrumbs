if(NOT DEFINED DATACRUMBS_SYSTEM_PROBE_FILE)
  message(FATAL_ERROR "DATACRUMBS_SYSTEM_PROBE_FILE is required")
endif()

if(NOT DEFINED DATACRUMBS_PROBE_SECRET_FILE)
  message(FATAL_ERROR "DATACRUMBS_PROBE_SECRET_FILE is required")
endif()

if(NOT DEFINED DATACRUMBS_SYSTEM_CONFIGURATOR_BIN)
  message(FATAL_ERROR "DATACRUMBS_SYSTEM_CONFIGURATOR_BIN is required")
endif()

if(EXISTS "${DATACRUMBS_SYSTEM_PROBE_FILE}" AND EXISTS "${DATACRUMBS_PROBE_SECRET_FILE}")
  message(
    STATUS "Install-time system configuration and secret already exist; skipping configurator"
  )
  return()
endif()

if(EXISTS "${DATACRUMBS_SYSTEM_PROBE_FILE}")
  message(STATUS "Install-time system configuration already exists; skipping configurator")
  return()
endif()

execute_process(COMMAND "${DATACRUMBS_SYSTEM_CONFIGURATOR_BIN}" RESULT_VARIABLE _dc_config_result)

if(NOT
   _dc_config_result
   EQUAL
   0
)
  message(FATAL_ERROR "datacrumbs_system_configurator failed with exit code ${_dc_config_result}")
endif()
