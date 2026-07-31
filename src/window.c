#include "engine_internal.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <siengine.h>
#include <stdint.h>

static void configure_swapchain(
    SIEngineCtx *ctx,
    SDL_Window *window,
    const SIWindow *window_desc
) {
    SDL_GPUPresentMode present_mode = SDL_GPU_PRESENTMODE_VSYNC;
    if (window_desc->vsync && SDL_WindowSupportsGPUPresentMode(
                                  ctx->primary_gpu,
                                  window,
                                  SDL_GPU_PRESENTMODE_MAILBOX
                              )) {
        present_mode = SDL_GPU_PRESENTMODE_MAILBOX;
    } else if (!window_desc->vsync) {
        if (SDL_WindowSupportsGPUPresentMode(
                ctx->primary_gpu,
                window,
                SDL_GPU_PRESENTMODE_IMMEDIATE
            )) {
            present_mode = SDL_GPU_PRESENTMODE_IMMEDIATE;
        } else if (
            SDL_WindowSupportsGPUPresentMode(
                ctx->primary_gpu,
                window,
                SDL_GPU_PRESENTMODE_MAILBOX
            )
        ) {
            present_mode = SDL_GPU_PRESENTMODE_MAILBOX;
        }
    }

    SDL_SetGPUSwapchainParameters(
        ctx->primary_gpu,
        window,
        SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
        present_mode
    );
}

static void on_window_set(const void *new_value) {
    SIEngineCtx *ctx = ecs_resource(SIEngineCtx);
    const SIWindow *window_desc = new_value;
    uint32_t width = window_desc->width ? window_desc->width : 1280;
    uint32_t height = window_desc->height ? window_desc->height : 720;

    if (ctx->window) {
        SDL_SetWindowTitle(ctx->window, window_desc->title);
        SDL_SetWindowResizable(ctx->window, window_desc->resizable);
        SDL_SetWindowSize(ctx->window, (int)width, (int)height);
        configure_swapchain(ctx, ctx->window, window_desc);
    }
}

static void EnsureWindow(ecs_iter_t *it) {
    SIEngineCtx *ctx = ecs_resource(SIEngineCtx);
    SIWindow *window_desc = ecs_try_get_resource(SIWindow);

    if (ctx->window || !window_desc)
        return;

    uint32_t width = window_desc->width ? window_desc->width : 1280;
    uint32_t height = window_desc->height ? window_desc->height : 720;
    SDL_WindowFlags flags = window_desc->resizable ? SDL_WINDOW_RESIZABLE : 0;

    ctx->window = SDL_CreateWindow(
        window_desc->title,
        (int)width,
        (int)height,
        flags | SDL_WINDOW_HIGH_PIXEL_DENSITY
    );
    SDL_ClaimWindowForGPUDevice(ctx->primary_gpu, ctx->window);

    configure_swapchain(ctx, ctx->window, window_desc);
    siui_attach_window();
}

static void PollWindowEvents(ecs_iter_t *it) {
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            ecs_quit();
            return;
        }

        siui_handle_event(&e);
    }
}

ECS_RESOURCE_DEFINE(SIWindow, .on_set = on_window_set);

void siwindow_register() {
    ECS_RESOURCE_REGISTER(SIWindow);
    ecs_system(
        {
            .name = "EnsureWindow",
            .phase = EcsPreRender,
            .callback = EnsureWindow,
        }
    );
    ecs_system(
        {
            .name = "PollWindowEvents",
            .phase = EcsPreRender,
            .callback = PollWindowEvents,
        }
    );
}

void siwindow_shutdown() {
    SIEngineCtx *ctx = ecs_resource(SIEngineCtx);

    if (ctx->window) {
        SDL_ReleaseWindowFromGPUDevice(ctx->primary_gpu, ctx->window);
        SDL_DestroyWindow(ctx->window);
        ctx->window = NULL;
    }
}
