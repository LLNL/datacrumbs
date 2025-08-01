#include <datacrumbs/common/singleton.h>
#include <datacrumbs/explorer/mechanism/ksym_capture.h>
template <>
std::shared_ptr<datacrumbs::KSymCapture> datacrumbs::Singleton<datacrumbs::KSymCapture>::instance =
    nullptr;
template <>
bool datacrumbs::Singleton<datacrumbs::KSymCapture>::stop_creating_instances = false;
