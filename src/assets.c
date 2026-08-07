#include "assets_internal.h"
#include <SDL3_image/SDL_image.h>
#include <stdlib.h>
#include <string.h>

enum {
    SI_HANDLE_GENERATION_MASK = 0x00ffffffu,
};

ECS_RESOURCE_DEFINE(SIAssetStore);

static SITextureHandle make_texture_handle(uint32_t index, uint32_t generation) {
    return ((uint64_t)(generation & SI_HANDLE_GENERATION_MASK) << 32) | index;
}

static uint32_t handle_index(SITextureHandle handle) {
    return (uint32_t)handle;
}

static uint32_t handle_generation(SITextureHandle handle) {
    return (uint32_t)((handle >> 32) & SI_HANDLE_GENERATION_MASK);
}

static uint32_t next_generation(uint32_t generation) {
    generation = (generation + 1) & SI_HANDLE_GENERATION_MASK;
    return generation ? generation : 1;
}

static void *grow_array(void *array, uint32_t *capacity, size_t element_size, uint32_t count) {
    if (count < *capacity)
        return array;

    uint32_t previous_capacity = *capacity;
    uint32_t next_capacity = *capacity ? *capacity * 2 : 16;
    while (next_capacity <= count)
        next_capacity *= 2;

    array = realloc(array, (size_t)next_capacity * element_size);
    memset(
        (char *)array + (size_t)previous_capacity * element_size,
        0,
        (size_t)(next_capacity - previous_capacity) * element_size
    );
    *capacity = next_capacity;
    return array;
}

static uint32_t alloc_texture_slot(SIAssetStore *assets) {
    if (assets->texture_free) {
        uint32_t index = assets->texture_free;
        assets->texture_free = assets->textures[index].next_free;
        return index;
    }

    uint32_t index = ++assets->texture_count;
    assets->textures = grow_array(
        assets->textures,
        &assets->texture_capacity,
        sizeof(*assets->textures),
        index
    );
    if (!assets->textures[index].generation)
        assets->textures[index].generation = 1;
    return index;
}

static SIAssetStore *assets_store(void) {
    return ecs_try_get_resource(SIAssetStore);
}

SITextureHandle siengine_load_texture(const char *path, SIFilterMode filter) {
    SIAssetStore *assets = assets_store();
    SIEngineCtx *engine = ecs_get_resource(SIEngineCtx);
    uint32_t index = alloc_texture_slot(assets);
    SITextureSlot *slot = &assets->textures[index];
    SDL_GPUCommandBuffer *command = SDL_AcquireGPUCommandBuffer(engine->primary_gpu);
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(command);
    int width = 0;
    int height = 0;
    SDL_GPUTexture *texture = IMG_LoadGPUTexture(
        engine->primary_gpu,
        copy,
        path,
        &width,
        &height
    );

    if (!texture) {
        SDL_EndGPUCopyPass(copy);
        SDL_CancelGPUCommandBuffer(command);
        slot->next_free = assets->texture_free;
        assets->texture_free = index;
        return SI_INVALID_HANDLE;
    }

    SDL_EndGPUCopyPass(copy);
    SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(command);
    SDL_WaitForGPUFences(engine->primary_gpu, true, &fence, 1);
    SDL_ReleaseGPUFence(engine->primary_gpu, fence);

    slot->gpu = texture;
    slot->width = (uint32_t)width;
    slot->height = (uint32_t)height;
    slot->filter = filter;
    slot->alive = true;
    return make_texture_handle(index, slot->generation);
}

bool siengine_texture_info(
    SITextureHandle handle,
    SDL_GPUTexture **texture,
    uint32_t *width,
    uint32_t *height,
    SIFilterMode *filter
) {
    SIAssetStore *assets = assets_store();
    uint32_t index = handle_index(handle);
    if (!assets || !index || index > assets->texture_count)
        return false;

    SITextureSlot *slot = &assets->textures[index];
    if (!slot->alive || slot->generation != handle_generation(handle))
        return false;

    if (texture)
        *texture = slot->gpu;
    if (width)
        *width = slot->width;
    if (height)
        *height = slot->height;
    if (filter)
        *filter = slot->filter;
    return true;
}

static void release_texture_slot(SIAssetStore *assets, uint32_t index) {
    SITextureSlot *slot = &assets->textures[index];
    SDL_GPUDevice *gpu = ecs_get_resource(SIEngineCtx)->primary_gpu;
    SDL_ReleaseGPUTexture(gpu, slot->gpu);
    slot->gpu = NULL;
    slot->alive = false;
    slot->generation = next_generation(slot->generation);
    slot->next_free = assets->texture_free;
    assets->texture_free = index;
}

void siengine_release_texture(SITextureHandle handle) {
    SIAssetStore *assets = assets_store();
    uint32_t index = handle_index(handle);
    if (!assets || !index || index > assets->texture_count)
        return;

    SITextureSlot *slot = &assets->textures[index];
    if (slot->alive && slot->generation == handle_generation(handle))
        release_texture_slot(assets, index);
}

void siassets_register(void) {
    ECS_RESOURCE_REGISTER(SIAssetStore);
    ecs_set_resource(SIAssetStore, {});
}

void siassets_shutdown(void) {
    SIAssetStore *assets = ecs_try_get_resource(SIAssetStore);
    if (!assets)
        return;

    for (uint32_t i = 1; i <= assets->texture_count; i++) {
        if (assets->textures[i].alive)
            release_texture_slot(assets, i);
    }

    free(assets->textures);
    assets->textures = NULL;
}
