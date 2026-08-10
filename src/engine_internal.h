#ifndef SIENGINE_INTERNAL_H
#define SIENGINE_INTERNAL_H

#include "siecs.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <stdbool.h>
#include <stdint.h>

ECS_RESOURCE_DECLARE(SIEngineCtx, {
    SDL_GPUDevice *primary_gpu;
    SDL_Window *window;
#if defined(__EMSCRIPTEN__)
    SDL_GLContext gl_context;
#endif
});

ECS_MODULE_DECLARE(siscene2d, {});
ECS_TAG_DECLARE(SIRenderable);
extern ecs_entity_t SI2DDefaultMaterial;

void sitransform_update_no_parent(ecs_iter_t *it);
void sitransform_update_parent(ecs_iter_t *it);
void sianimation_update(ecs_iter_t *it);

void siwindow_register();
void siwindow_poll_events(void);
void siassets_register();
void sirender_register();
SIEngineCtx siplatform_init(void);
void siplatform_shutdown(SIEngineCtx *ctx);
void siplatform_run(void);

#endif
