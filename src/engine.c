#include "siecs.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <siengine.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

ECS_MODULE_DEFINE(siengine);

ECS_RESOURCE_DECLARE(SIEngineCtx, {
    SDL_GPUDevice *primary_gpu;
    SDL_GPUCommandBuffer *cmd;
});

ECS_RESOURCE_DEFINE(SIEngineCtx);

void on_window_remove(ecs_world_t *world, ecs_entity_t, ecs_component_t, void *data) {
    SIEngineCtx *ctx = ecs_resource(world, SIEngineCtx);
    SIWindow *window = data;

    SDL_ReleaseWindowFromGPUDevice(ctx->primary_gpu, (SDL_Window *) window->ptr);
    SDL_DestroyWindow((SDL_Window *) window->ptr);
}

ECS_COMPONENT_DEFINE(SIWindow, .on_remove = on_window_remove);

void begin_drawing(ecs_iter_t *it) {
    SIEngineCtx *ctx = ecs_resource(it->world, SIEngineCtx);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            exit(0);
        }

        if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            SDL_WindowID closed_id = e.window.windowID;
            ecs_query_id_t q = ecs_query(it->world, { .terms = { ecs_inout(SIWindow) } });
            ecs_iter_t q_it = ecs_query_iter(it->world, q);

            while (ecs_iter_next(&q_it)) {
                SIWindow *windows = ecs_field(&q_it, 0);
                for (uint32_t i = 0; i < q_it.count; i++) {
                    if (SDL_GetWindowID((SDL_Window *)windows[i].ptr) == closed_id) {
                        ecs_kill(it->world, q_it.entities[i]);
                        break;
                    }
                }
            }

            ecs_query_fini(it->world, q);
        }
    }

    ctx->cmd = SDL_AcquireGPUCommandBuffer(ctx->primary_gpu);
}

void drawing(ecs_iter_t *it) {
    SIEngineCtx *ctx = ecs_resource(it->world, SIEngineCtx);
    SIWindow *windows = ecs_field(it, 0);
    SDL_GPUTexture *swapchain_texture = NULL;
    uint32_t width, height;

    for (uint32_t i = 0; i < it->count; i++) {
        SDL_WaitAndAcquireGPUSwapchainTexture(
            ctx->cmd,
            (SDL_Window *)windows[i].ptr,
            &swapchain_texture,
            &width,
            &height
        );

        SDL_GPUColorTargetInfo color_target = {
            .texture = swapchain_texture,
            .clear_color = { 0.05f, 0.05f, 0.08f, 1.0f },
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
        };

        SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(ctx->cmd, &color_target, 1, NULL);

        SDL_EndGPURenderPass(pass);
    }
}

void end_drawing(ecs_iter_t *it) {
    SIEngineCtx *ctx = ecs_resource(it->world, SIEngineCtx);
    SDL_SubmitGPUCommandBuffer(ctx->cmd);
}

SIWindow siengine_create_window(ecs_world_t *world, const char *title) {
    SIEngineCtx *ctx = ecs_resource(world, SIEngineCtx);

    SDL_Window *window = SDL_CreateWindow(title, 1920, 1080, SDL_WINDOW_RESIZABLE);

    SDL_ClaimWindowForGPUDevice(ctx->primary_gpu, window);

    return (SIWindow){ .ptr = (uint64_t)window };
}

void siengine_import(ecs_world_t *world, const siengine_props_t *props) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GPUDevice *gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, "vulkan");

    ECS_RESOURCE_REGISTER(world, SIEngineCtx);
    ECS_COMPONENT_REGISTER(world, SIWindow);

    ecs_set_resource(world, SIEngineCtx, { .primary_gpu = gpu });

    ecs_system(world, { .name = "BeginDrawing", .phase = EcsPreRender, .callback = begin_drawing });
    ecs_system(
        world,
        { .name = "Drawing",
          .query.terms = { ecs_inout(SIWindow) },
          .phase = EcsPreRender,
          .callback = drawing }
    );
    ecs_system(
        world,
        { .name = "EndDrawing", .query.terms = {}, .phase = EcsPreRender, .callback = end_drawing }
    );
}
