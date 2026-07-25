#include "engine_internal.h"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <siengine.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void on_window_remove(ecs_entity_t entity, ecs_component_t component, void *data) {
    SIEngineCtx *ctx = ecs_resource(SIEngineCtx);
    SIWindowHandle *handle = ecs_get(entity, SIWindowHandle);
    SIWindow *window_desc = data;

    SDL_ReleaseWindowFromGPUDevice(ctx->primary_gpu, handle->handle);
    SDL_DestroyWindow(handle->handle);
    ecs_remove(entity, SIWindowHandle);

    free(window_desc->title);
}

static void on_window_set(
    ecs_entity_t entity,
    ecs_component_t component,
    const void *new_value,
    void *old_value
) {
    SIEngineCtx *ctx = ecs_resource(SIEngineCtx);
    const SIWindow *window_desc = new_value;

    SIWindowHandle *handle = ecs_try_get(entity, SIWindowHandle);

    SIWindow *old_desc = old_value;
    if (old_desc->title) {
        free(old_desc->title);
    }

    uint32_t width = window_desc->width ? window_desc->width : 1280;
    uint32_t height = window_desc->height ? window_desc->height : 720;

    if (handle) {
        SDL_SetWindowTitle(handle->handle, window_desc->title);
        SDL_SetWindowResizable(handle->handle, window_desc->resizable);
        SDL_SetWindowSize(handle->handle, (int)width, (int)height);
        handle->width = width;
        handle->height = height;
    } else {
        SDL_WindowFlags flags = window_desc->resizable ? SDL_WINDOW_RESIZABLE : 0;
        SDL_Window *window = SDL_CreateWindow(window_desc->title, (int)width, (int)height, flags);
        SDL_ClaimWindowForGPUDevice(ctx->primary_gpu, window);

        SDL_GPUPresentMode present_mode = SDL_GPU_PRESENTMODE_VSYNC;
        if (window_desc->vsync &&
            SDL_WindowSupportsGPUPresentMode(ctx->primary_gpu, window, SDL_GPU_PRESENTMODE_MAILBOX)) {
            present_mode = SDL_GPU_PRESENTMODE_MAILBOX;
        } else if (!window_desc->vsync) {
            if (SDL_WindowSupportsGPUPresentMode(
                    ctx->primary_gpu, window, SDL_GPU_PRESENTMODE_IMMEDIATE
                )) {
                present_mode = SDL_GPU_PRESENTMODE_IMMEDIATE;
            } else if (SDL_WindowSupportsGPUPresentMode(
                           ctx->primary_gpu, window, SDL_GPU_PRESENTMODE_MAILBOX
                       )) {
                present_mode = SDL_GPU_PRESENTMODE_MAILBOX;
            }
        }

        SDL_SetGPUSwapchainParameters(
            ctx->primary_gpu, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, present_mode
        );

        ecs_set(entity, SIWindowHandle, { .handle = window, .width = width, .height = height });
    }
}

static void PollWindowEvents(ecs_iter_t *it) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            ecs_quit();
            return;
        }

        if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            SDL_WindowID closed_id = e.window.windowID;
            ecs_query_id_t q = ecs_query({ .terms = { ecs_inout(SIWindowHandle) } });
            ecs_iter_t q_it = ecs_query_iter(q);

            while (ecs_iter_next(&q_it)) {
                SIWindowHandle *windows = ecs_field(&q_it, 0);
                for (uint32_t i = 0; i < q_it.count; i++) {
                    if (SDL_GetWindowID(windows[i].handle) == closed_id) {
                        ecs_kill(q_it.entities[i]); // window is destroy on remove
                        ecs_quit();
                        break;
                    }
                }
            }

            ecs_query_fini(q);
        }
    }
}

ECS_COMPONENT_DEFINE(SIWindow, .on_remove = on_window_remove, .on_set = on_window_set);
ECS_COMPONENT_DEFINE(SIWindowHandle);

void siwindow_register() {
    ECS_COMPONENT_REGISTER(SIWindow);
    ECS_COMPONENT_REGISTER(SIWindowHandle);

    ecs_system(
        {
            .name = "PollWindowEvents",
            .phase = EcsPreRender,
            .callback = PollWindowEvents,
        }
    );
}
