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

ECS_MODULE_DECLARE(sitransform, {});

void siwindow_register();
void sirender_register();
void sirender_shutdown();
void siui_register();
void siui_shutdown();
void siwindow_shutdown();
void siui_attach_window();
bool siui_handle_event(const SDL_Event *event);
bool siui_render(
    SDL_GPUCommandBuffer *command,
    SDL_GPUTexture *texture,
    uint32_t pixel_width,
    uint32_t pixel_height,
    SDL_FColor clear_color,
    bool clear_target
);

#endif
