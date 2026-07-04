#include "engine_internal.h"
#include "siecs.h"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <siengine.h>
#include <stdio.h>

ECS_MODULE_DEFINE(siengine);

static void on_engine_remove(ecs_world_t *world, const void *ptr) {
    SIEngineCtx *ctx = (SIEngineCtx *)ptr;
    sirender_shutdown(world, ctx);
    if (ctx->primary_gpu != NULL) {
        SDL_DestroyGPUDevice(ctx->primary_gpu);
    }
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

ECS_RESOURCE_DEFINE(SIEngineCtx, .on_remove = on_engine_remove);

void siengine_import(ecs_world_t *world, const siengine_props_t *props) {
    (void)props;

    ECS_MODULE_IMPORT(world, sitransform, {});
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GPUDevice *gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, "vulkan");
    if (gpu == NULL) {
        fprintf(stderr, "siengine: SDL_CreateGPUDevice failed: %s\n", SDL_GetError());
    }

    ECS_RESOURCE_REGISTER(world, SIEngineCtx);
    ecs_set_resource(world, SIEngineCtx, { .primary_gpu = gpu });

    siwindow_register(world);
    sirender_register(world);
}
