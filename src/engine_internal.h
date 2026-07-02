#ifndef SIENGINE_INTERNAL_H
#define SIENGINE_INTERNAL_H

#include "siecs.h"
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>

ECS_RESOURCE_DECLARE(SIEngineCtx, {
    SDL_GPUDevice *primary_gpu;
    SDL_GPUCommandBuffer *cmd;
});

ECS_COMPONENT_DECLARE(SIWindowHandle, { SDL_Window *handle; });

void siwindow_register(ecs_world_t *world);
void sirender_register(ecs_world_t *world);

#endif
