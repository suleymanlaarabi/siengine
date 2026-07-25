#include "render_internal.h"
#include <SDL3/SDL_error.h>
#include <stdio.h>

void sirender_draw_windows(ecs_iter_t *it) {
    SIEngineCtx *engine = ecs_resource(SIEngineCtx);
    SIRenderState *render = ecs_resource(SIRenderState);
    SIRenderFrame *frame = &render->frame;
    SICubeRenderState *cubes = &render->cubes;
    SIWindowHandle *windows = ecs_field(it, 0);

    if (frame->view_count == 0) {
        return;
    }

    for (uint32_t i = 0; i < it->count; i++) {
        SDL_GPUTexture *swapchain_texture;
        uint32_t window_width, window_height;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(
                frame->cmd,
                windows[i].handle,
                &swapchain_texture,
                &window_width,
                &window_height
            )) {
            fprintf(
                stderr,
                "siengine: SDL_WaitAndAcquireGPUSwapchainTexture failed: %s\n",
                SDL_GetError()
            );
            continue;
        }
        if (swapchain_texture == NULL || window_width == 0 || window_height == 0) {
            continue;
        }

        SDL_GPUTextureFormat color_format =
            SDL_GetGPUSwapchainTextureFormat(engine->primary_gpu, windows[i].handle);
        if (!sicube_ensure_pipeline(color_format)) {
            continue;
        }

        SDL_GPUTexture *depth =
            siwindow_ensure_depth_target(windows[i].handle, window_width, window_height);
        if (depth == NULL) {
            continue;
        }

        SDL_GPUColorTargetInfo color_target = {
            .texture = swapchain_texture,
            .clear_color = { 0.05f, 0.05f, 0.08f, 1.0f },
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
        };
        SDL_GPUDepthStencilTargetInfo depth_target = {
            .texture = depth,
            .clear_depth = 1.0f,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_DONT_CARE,
        };

        SDL_GPURenderPass *pass =
            SDL_BeginGPURenderPass(frame->cmd, &color_target, 1, &depth_target);
        SDL_BindGPUGraphicsPipeline(pass, cubes->pipeline);

        for (uint32_t view_i = 0; view_i < frame->view_count; view_i++) {
            SIRenderView *view = &frame->views[view_i];
            float aspect = (float)window_width / (float)window_height;
            float fov_y = view->camera.fov_y > 0.0f ? view->camera.fov_y : 1.0471976f;
            float near_clip = view->camera.near_clip > 0.0f ? view->camera.near_clip : 0.1f;
            float far_clip = view->camera.far_clip > near_clip ? view->camera.far_clip : 1000.0f;
            SIMat4 projection = si_mat4_perspective(fov_y, aspect, near_clip, far_clip);
            SIMat4 view_projection = si_mat4_mul(projection, view->view);

            sicube_draw_instances(pass, view_projection);
        }

        SDL_EndGPURenderPass(pass);
    }
}
