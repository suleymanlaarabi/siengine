#include "engine_internal.h"
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <siengine.h>

ECS_MODULE_DEFINE(siengine);

ECS_RESOURCE_DEFINE(SIEngineCtx);

void siengine_import(ecs_world_t *world, const siengine_props_t *props) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GPUDevice *gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, "vulkan");

    ECS_RESOURCE_REGISTER(world, SIEngineCtx);
    ecs_set_resource(world, SIEngineCtx, { .primary_gpu = gpu });

    siwindow_register(world);
    sirender_register(world);
}
