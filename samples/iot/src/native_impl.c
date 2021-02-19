
#include "bh_platform.h"
#include "wasm_export.h"

extern bool
wasm_runtime_call_indirect(wasm_exec_env_t exec_env,
                           uint32_t element_indices,
                           uint32_t argc,
                           uint32_t argv[]);

void
IMP_set_on(wasm_exec_env_t exec_env, int onoff)
{
    printf("calling into native function: %s\n", __FUNCTION__);
}

void
IMP_set_hue(wasm_exec_env_t exec_env, int hue)
{
    printf("calling into native function: %s\n", __FUNCTION__);
}

void
IMP_set_brightness(wasm_exec_env_t exec_env, int32_t brightness)
{
    printf("calling into native function: %s\n", __FUNCTION__);
}
