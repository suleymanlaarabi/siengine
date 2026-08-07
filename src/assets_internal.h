#ifndef SIENGINE_ASSETS_INTERNAL_H
#define SIENGINE_ASSETS_INTERNAL_H

#include "engine_internal.h"
#include "siengine.h"
#include <SDL3/SDL_gpu.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    SDL_GPUTexture *gpu;
    uint32_t width;
    uint32_t height;
    uint32_t generation;
    uint32_t next_free;
    SIFilterMode filter;
    bool alive;
} SITextureSlot;

ECS_RESOURCE_DECLARE(SIAssetStore, {
    SITextureSlot *textures;
    uint32_t texture_count;
    uint32_t texture_capacity;
    uint32_t texture_free;
});

void siassets_register(void);
void siassets_shutdown(void);

bool siengine_texture_info(
    SITextureHandle handle,
    SDL_GPUTexture **texture,
    uint32_t *width,
    uint32_t *height,
    SIFilterMode *filter
);

#endif
