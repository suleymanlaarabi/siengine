#include "engine_internal.h"
#include "siecs.h"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <siengine.h>
#include <stdio.h>

ECS_MODULE_DEFINE(siengine);

static void on_engine_remove(const void *ptr) {
    SIEngineCtx *ctx = (SIEngineCtx *)ptr;
    sirender_shutdown();
    SDL_DestroyGPUDevice(ctx->primary_gpu);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

ECS_RESOURCE_DEFINE(SIEngineCtx, .on_remove = on_engine_remove);

void siengine_import(const siengine_props_t *) {
    ECS_MODULE_IMPORT(sitransform, {});
    SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "wayland,x11", SDL_HINT_OVERRIDE);
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    fprintf(stderr, "siengine: SDL video driver: %s\n", SDL_GetCurrentVideoDriver());

#ifdef NDEBUG
    const bool gpu_debug = false;
#else
    const bool gpu_debug = true;
#endif
    SDL_GPUDevice *gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, gpu_debug, "vulkan");

    ECS_RESOURCE_REGISTER(SIEngineCtx);
    ecs_set_resource(SIEngineCtx, { .primary_gpu = gpu });

    siwindow_register();
    sirender_register();
}
