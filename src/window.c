#include "engine_internal.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <siengine.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void
on_window_remove(ecs_world_t *world, ecs_entity_t entity, ecs_component_t component, void *data) {
    SIEngineCtx *ctx = ecs_resource(world, SIEngineCtx);
    SIWindowHandle *handle = ecs_get(world, entity, SIWindowHandle);
    SIWindow *window_desc = data;

    if (handle != NULL && handle->handle != NULL) {
        SDL_ReleaseWindowFromGPUDevice(ctx->primary_gpu, handle->handle);
        SDL_DestroyWindow(handle->handle);
        ecs_remove(world, entity, SIWindowHandle);
    }

    free(window_desc->title);
}

static void on_window_set(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_component_t component,
    const void *new_value,
    void *old_value
) {
    SIEngineCtx *ctx = ecs_resource(world, SIEngineCtx);
    const SIWindow *window_desc = new_value;

    SIWindowHandle *handle = ecs_try_get(world, entity, SIWindowHandle);

    SIWindow *old_desc = old_value;
    if (old_desc->title) {
        free(old_desc->title);
    }

    if (handle) {
        SDL_SetWindowTitle(handle->handle, window_desc->title);
    } else {
        SDL_Window *window = SDL_CreateWindow(window_desc->title, 1280, 720, SDL_WINDOW_RESIZABLE);
        SDL_ClaimWindowForGPUDevice(ctx->primary_gpu, window);

        ecs_set(world, entity, SIWindowHandle, { .handle = window });
    }
}

static void PollWindowEvents(ecs_iter_t *it) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            exit(0);
        }

        if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            SDL_WindowID closed_id = e.window.windowID;
            ecs_query_id_t q = ecs_query(it->world, { .terms = { ecs_inout(SIWindowHandle) } });
            ecs_iter_t q_it = ecs_query_iter(it->world, q);

            while (ecs_iter_next(&q_it)) {
                SIWindowHandle *windows = ecs_field(&q_it, 0);
                for (uint32_t i = 0; i < q_it.count; i++) {
                    if (SDL_GetWindowID(windows[i].handle) == closed_id) {
                        ecs_kill(it->world, q_it.entities[i]); // window is destroy on remove
                        break;
                    }
                }
            }

            ecs_query_fini(it->world, q);
        }
    }
}

ECS_COMPONENT_DEFINE(SIWindow, .on_remove = on_window_remove, .on_set = on_window_set);
ECS_COMPONENT_DEFINE(SIWindowHandle);

void siwindow_register(ecs_world_t *world) {
    ECS_COMPONENT_REGISTER(world, SIWindow);
    ECS_COMPONENT_REGISTER(world, SIWindowHandle);

    ecs_system(
        world,
        {
            .name = "PollWindowEvents",
            .phase = EcsPreRender,
            .callback = PollWindowEvents,
        }
    );
}
