#ifndef SIENGINE_ASSETS_INTERNAL_H
#define SIENGINE_ASSETS_INTERNAL_H

#include "engine_internal.h"
#include "siengine.h"
#include <stdint.h>

ECS_COMPONENT_DECLARE(SITexture, {
    uint64_t gpu_handle;
    uint32_t width;
    uint32_t height;
});

void siassets_register(void);
void siassets_shutdown(void);

#endif
