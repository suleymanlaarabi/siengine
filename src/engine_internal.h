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
});

ECS_MODULE_DECLARE(siscene2d, {});

void siwindow_register();
void siassets_register();
void siassets_shutdown();
void sirender_register();
void sirender_shutdown();
void siwindow_shutdown();

#endif
