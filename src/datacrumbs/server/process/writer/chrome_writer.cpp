

#include <datacrumbs/common/logging.h>  // Use datacrumbs logging macros
#include <datacrumbs/common/singleton.h>
#include <datacrumbs/server/process/writer/chrome_writer.h>

// Specialization of the Singleton instance for KSymCapture.
// This holds the shared pointer to the singleton instance.
template <>
std::shared_ptr<datacrumbs::ChromeWriter>
    datacrumbs::Singleton<datacrumbs::ChromeWriter>::instance = nullptr;

// Specialization of the flag to stop creating new instances of KSymCapture.
template <>
bool datacrumbs::Singleton<datacrumbs::ChromeWriter>::stop_creating_instances = false;
