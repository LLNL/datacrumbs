// ksym_capture.cpp
// This file implements the Singleton specialization for KSymCapture.
// It also sets up logging using the datacrumbs logging system.
#include <datacrumbs/explorer/mechanism/ksym_capture.h>

// Specialization of the Singleton instance for KSymCapture.
// This holds the shared pointer to the singleton instance.
template <>
std::shared_ptr<datacrumbs::KSymCapture> datacrumbs::Singleton<datacrumbs::KSymCapture>::instance =
    nullptr;

// Specialization of the flag to stop creating new instances of KSymCapture.
template <>
bool datacrumbs::Singleton<datacrumbs::KSymCapture>::stop_creating_instances = false;
