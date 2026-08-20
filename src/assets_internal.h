#ifndef SIENGINE_ASSETS_INTERNAL_H
#define SIENGINE_ASSETS_INTERNAL_H

#include "engine_internal.h"
#include "siengine.h"
#include <stdint.h>

typedef enum {
    SI_TEXTURE_PENDING,
    SI_TEXTURE_READY,
} SITextureState;

ECS_COMPONENT_DECLARE(SITexture, {
    uint64_t gpu_handle;
    uint32_t width;
    uint32_t height;
    uint8_t filter;
    uint8_t state;
    char path[512];
});

ECS_TAG_DECLARE(SITextureRelease);

void siassets_register(void);

#endif
