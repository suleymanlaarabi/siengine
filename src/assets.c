#include "assets_internal.h"
#include <SDL3/SDL_filesystem.h>
#include <SDL3_image/SDL_image.h>
#include <stdlib.h>
#include <string.h>
#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#include <emscripten/html5_webgl.h>
#endif

#define SI_HANDLE_GENERATION_MASK 0x00ffffffu

ECS_RESOURCE_DEFINE(SIAssetStore);
ECS_RESOURCE_DEFINE(SIAssetRoot);

static void append_asset_path(char destination[512], const char *path) {
    size_t length = strlen(destination);
    if (length && destination[length - 1] != '/')
        SDL_strlcat(destination, "/", 512);
    SDL_strlcat(destination, path, 512);
}

static const char *resolve_asset_path(const char *path, char resolved[512]) {
    if (path[0] == '/')
        return path;

    SIAssetRoot *root = ecs_get_resource(SIAssetRoot);
    resolved[0] = '\0';
    if (root->path[0] != '/')
        append_asset_path(resolved, SDL_GetBasePath());
    append_asset_path(resolved, root->path);
    append_asset_path(resolved, path);
    return resolved;
}

static SITextureHandle make_texture_handle(uint32_t index, uint32_t generation) {
    return ((uint64_t)(generation & SI_HANDLE_GENERATION_MASK) << 32) | index;
}

static uint32_t handle_index(SITextureHandle handle) { return (uint32_t)handle; }

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
    assets->textures =
        grow_array(assets->textures, &assets->texture_capacity, sizeof(*assets->textures), index);
    if (!assets->textures[index].generation)
        assets->textures[index].generation = 1;
    return index;
}

static SIAssetStore *assets_store(void) { return ecs_try_get_resource(SIAssetStore); }

#if defined(__EMSCRIPTEN__)
static GLuint load_webgl_texture(const char *path, SIFilterMode filter, int *width, int *height) {
    SDL_Surface *loaded = IMG_Load(path);
    SDL_Surface *surface = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    *width = surface->w;
    *height = surface->h;
    SDL_DestroySurface(loaded);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        filter == SI_FILTER_LINEAR ? GL_LINEAR : GL_NEAREST
    );
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        filter == SI_FILTER_LINEAR ? GL_LINEAR : GL_NEAREST
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        surface->w,
        surface->h,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        surface->pixels
    );
    SDL_DestroySurface(surface);
    return texture;
}
#endif

SITextureHandle siengine_load_texture(const char *path, SIFilterMode filter) {
    SIAssetStore *assets = assets_store();
#if !defined(__EMSCRIPTEN__)
    SIEngineCtx *engine = ecs_get_resource(SIEngineCtx);
#endif
    char resolved_path[512];
    path = resolve_asset_path(path, resolved_path);
    uint32_t index = alloc_texture_slot(assets);
    SITextureSlot *slot = &assets->textures[index];
#if defined(__EMSCRIPTEN__)
    SIEngineCtx *engine = ecs_resource(SIEngineCtx);
    siwindow_ensure();
    emscripten_webgl_make_context_current(
        (EMSCRIPTEN_WEBGL_CONTEXT_HANDLE)(uintptr_t)engine->gl_context
    );
    int width = 0;
    int height = 0;
    slot->webgl = load_webgl_texture(path, filter, &width, &height);
#else
    SDL_GPUCommandBuffer *command = SDL_AcquireGPUCommandBuffer(engine->primary_gpu);
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(command);
    int width = 0;
    int height = 0;
    SDL_GPUTexture *texture = IMG_LoadGPUTexture(engine->primary_gpu, copy, path, &width, &height);

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
#endif
    slot->width = (uint32_t)width;
    slot->height = (uint32_t)height;
    slot->filter = filter;
    slot->alive = true;
    return make_texture_handle(index, slot->generation);
}

SITextureSlot *siengine_texture_slot(SITextureHandle handle) {
    return &ecs_resource(SIAssetStore)->textures[handle_index(handle)];
}

static void release_texture_slot(SIAssetStore *assets, uint32_t index) {
    SITextureSlot *slot = &assets->textures[index];
#if defined(__EMSCRIPTEN__)
    glDeleteTextures(1, &slot->webgl);
    slot->webgl = 0;
#else
    SDL_GPUDevice *gpu = ecs_get_resource(SIEngineCtx)->primary_gpu;
    SDL_ReleaseGPUTexture(gpu, slot->gpu);
    slot->gpu = NULL;
#endif
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
    ECS_RESOURCE_REGISTER(SIAssetRoot);
    ecs_set_resource(SIAssetRoot, { .path = "./assets" });

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
