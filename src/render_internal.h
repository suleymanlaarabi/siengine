#ifndef SIENGINE_RENDER_INTERNAL_H
#define SIENGINE_RENDER_INTERNAL_H

#include "engine_internal.h"
#include "siengine.h"
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    SDL_GPUCommandBuffer *cmd;
} SIRenderFrame;

ECS_RESOURCE_DECLARE(SIRenderState, {
    SIRenderFrame frame;
});

void sirender_begin_frame(ecs_iter_t *it);
void sirender_end_frame(ecs_iter_t *it);
void sirender_draw_window(ecs_iter_t *it);
void sirender_frame_shutdown();

#endif
