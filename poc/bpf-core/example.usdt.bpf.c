#include "example.generic.bpf.h"
SEC("usdt//usr/lib64/libpython3.11.so:python:function__entry")
int BPF_USDT(python_function_entry, void* class, void* function) {
  generic_call(7);
  return 0;
}
SEC("usdt//usr/lib64/libpython3.11.so:python:function__return")
int BPF_USDT(python_function_return, void* class, void* function) {
  generic_call(8);
  return 0;
}