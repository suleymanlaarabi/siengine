#include "engine_internal.h"
#include <SDL3/SDL_gpu.h>
#include <stdint.h>

static void begin_drawing(ecs_iter_t *it) {
    SIEngineCtx *ctx = ecs_resource(it->world, SIEngineCtx);

    ctx->cmd = SDL_AcquireGPUCommandBuffer(ctx->primary_gpu);
}

static void drawing(ecs_iter_t *it) {
    SIEngineCtx *ctx = ecs_resource(it->world, SIEngineCtx);
    SIWindowHandle *windows = ecs_field(it, 0);

    for (uint32_t i = 0; i < it->count; i++) {
        SDL_GPUTexture *swapchain_texture;

        uint32_t window_width, window_height;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(
                ctx->cmd,
                windows[i].handle,
                &swapchain_texture,
                &window_width,
                &window_height
            )) {
            continue;
        }

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

static void end_drawing(ecs_iter_t *it) {
    SIEngineCtx *ctx = ecs_resource(it->world, SIEngineCtx);

    SDL_SubmitGPUCommandBuffer(ctx->cmd);
}

void sirender_register(ecs_world_t *world) {
    ecs_system(
        world,
        {
            .name = "BeginDrawing",
            .phase = EcsPreRender,
            .callback = begin_drawing,
        }
    );
    ecs_system(
        world,
        {
            .name = "Drawing",
            .query.terms = { ecs_inout(SIWindowHandle) },
            .phase = EcsPreRender,
            .callback = drawing,
        }
    );
    ecs_system(
        world,
        {
            .name = "EndDrawing",
            .query.terms = {},
            .phase = EcsPreRender,
            .callback = end_drawing,
        }
    );
}
