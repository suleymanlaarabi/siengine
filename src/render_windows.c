#include "render_internal.h"

void sirender_draw_window(ecs_iter_t *it) {
    SIEngineCtx *engine = ecs_resource(SIEngineCtx);
    SIRenderState *render = ecs_resource(SIRenderState);
    SIRenderFrame *frame = &render->frame;
    SDL_GPUTexture *swapchain_texture = NULL;
    uint32_t pixel_width = 0;
    uint32_t pixel_height = 0;

    if (!engine->window)
        return;

    if (!SDL_WaitAndAcquireGPUSwapchainTexture(
            frame->cmd,
            engine->window,
            &swapchain_texture,
            &pixel_width,
            &pixel_height
        )) {
        SDL_CancelGPUCommandBuffer(frame->cmd);
        frame->cmd = NULL;
        return;
    }

    if (!swapchain_texture || pixel_width == 0 || pixel_height == 0)
        return;

    SDL_GPUColorTargetInfo color_target = {
        .texture = swapchain_texture,
        .clear_color = { 0.05f, 0.05f, 0.08f, 1.0f },
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
    };

    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(frame->cmd, &color_target, 1, NULL);
    if (!pass) {
        SDL_CancelGPUCommandBuffer(frame->cmd);
        frame->cmd = NULL;
        return;
    }
    SDL_EndGPURenderPass(pass);

    if (!siui_render(
            frame->cmd,
            swapchain_texture,
            pixel_width,
            pixel_height,
            (SDL_FColor){ 0.05f, 0.05f, 0.08f, 1.0f },
            false
        )) {
        SDL_CancelGPUCommandBuffer(frame->cmd);
        frame->cmd = NULL;
    }
}
