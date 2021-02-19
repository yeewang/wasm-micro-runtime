
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../src/interface.h"

void
set_on(int32_t onoff)
{
    printf("calling into WASM function: %s\n", __FUNCTION__);

    IMP_set_on(onoff != 0);
}

void
set_hue(int32_t hue)
{
    printf("calling into WASM function: %s\n", __FUNCTION__);

    IMP_set_hue(hue);
}

void
set_brightness(int32_t brightness)
{
    printf("calling into WASM function: %s\n", __FUNCTION__);

    IMP_set_brightness(brightness);
}
