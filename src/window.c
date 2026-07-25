#include "engine_internal.h"
#include "siecs.h"
#include "siui.h"
#include "siui_sdl_gpu.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <siengine.h>
#include <stdint.h>

ECS_RESOURCE_DEFINE(SIUI);

static void on_window_handle_remove(ecs_entity_t entity, ecs_component_t component, void *data) {
    SIEngineCtx *ctx = ecs_resource(SIEngineCtx);
    SIWindowHandle *handle = data;

    SDL_ReleaseWindowFromGPUDevice(ctx->primary_gpu, handle->handle);
    SDL_DestroyWindow(handle->handle);
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

    uint32_t width = window_desc->width ? window_desc->width : 1280;
    uint32_t height = window_desc->height ? window_desc->height : 720;

    if (handle) {
        SDL_SetWindowTitle(handle->handle, window_desc->title);
        SDL_SetWindowResizable(handle->handle, window_desc->resizable);
        SDL_SetWindowSize(handle->handle, (int)width, (int)height);
    } else {
        SDL_WindowFlags flags = window_desc->resizable ? SDL_WINDOW_RESIZABLE : 0;
        SDL_Window *window = SDL_CreateWindow(
            window_desc->title,
            (int)width,
            (int)height,
            flags | SDL_WINDOW_HIGH_PIXEL_DENSITY
        );
        SDL_ClaimWindowForGPUDevice(ctx->primary_gpu, window);

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

        if (window_desc->ui) {
            TTF_Font *font = TTF_OpenFont("/usr/share/fonts/noto/NotoSans-Regular.ttf", 16.0f);

            siui_sdlgpu_t *ui = siui_sdlgpu_create(&(siui_sdlgpu_desc_t){
                .device = ctx->primary_gpu,
                .window = window,
                .default_font = font,
                .default_font_size = 16.0f,
            });

            ecs_set_resource(SIUI, { .ui = ui });

            siui_mount_impl(window_desc->ui, NULL, 0);
        }

        ecs_set(entity, SIWindowHandle, { .handle = window });
    }
}

static void RemoveOrphanedWindowHandles(ecs_iter_t *it) {
    for (uint32_t i = 0; i < it->count; i++) {
        ecs_remove(it->entities[i], SIWindowHandle);
    }
}

static bool dispatch_siui_event(const SDL_Event *event, bool *handled) {
    if (!event) {
        return false;
    }

    if (handled) {
        *handled = false;
    }

    siinput_event_t input = { 0 };

    switch (event->type) {
    case SDL_EVENT_MOUSE_MOTION:
        input.type = SIUI_EVENT_POINTER_MOVE;
        input.x = event->motion.x;
        input.y = event->motion.y;
        input.buttons = event->motion.state;
        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        input.type = SIUI_EVENT_POINTER_DOWN;
        input.x = event->button.x;
        input.y = event->button.y;
        input.button = event->button.button;
        input.clicks = event->button.clicks;
        break;

    case SDL_EVENT_MOUSE_BUTTON_UP:
        input.type = SIUI_EVENT_POINTER_UP;
        input.x = event->button.x;
        input.y = event->button.y;
        input.button = event->button.button;
        input.clicks = event->button.clicks;
        break;

    case SDL_EVENT_MOUSE_WHEEL:
        input.type = SIUI_EVENT_SCROLL;
        input.delta_x = event->wheel.x;
        input.delta_y = event->wheel.y;
        break;

    default:
        return true;
    }

    bool event_handled = siui_dispatch(&input);
    if (handled) {
        *handled = event_handled;
    }
    return true;
}

static void PollWindowEvents(ecs_iter_t *it) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            ecs_quit();
            return;
        }

        dispatch_siui_event(&e, NULL);

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

ECS_COMPONENT_DEFINE(SIWindow, .on_set = on_window_set);
ECS_COMPONENT_DEFINE(SIWindowHandle, .on_remove = on_window_handle_remove);

void siwindow_register() {
    ECS_COMPONENT_REGISTER(SIWindow);
    ECS_COMPONENT_REGISTER(SIWindowHandle);
    ECS_RESOURCE_REGISTER(SIUI);

    ecs_set_resource(SIUI, { .ui = NULL });

    ecs_system(
        {
            .name = "RemoveOrphanedWindowHandles",
            .query.terms = { ecs_filter(SIWindowHandle), ecs_not(SIWindow) },
            .phase = EcsPreRender,
            .callback = RemoveOrphanedWindowHandles,
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
