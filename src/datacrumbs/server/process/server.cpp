#include <datacrumbs/common/logging.h>
#include <datacrumbs/server/bpf/datacrumbs.skel.h>

#include <datacrumbs/server/process/server.impl.cpp>

int main(int argc, char** argv) {
#ifdef DATACRUMBS_DISABLE_PROBE_SIGNING
  DC_LOG_WARN("Probe signing is disabled. datacrumbs will accept unsigned probes files.");
#endif
  return main_call(argc, argv);
}