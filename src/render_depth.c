#include "render_internal.h"
#include <SDL3/SDL_error.h>
#include <stdio.h>

SDL_GPUTexture *siwindow_ensure_depth_target(SDL_Window *window, uint32_t width, uint32_t height) {
    SIEngineCtx *engine = ecs_resource(SIEngineCtx);
    SIRenderState *render = ecs_resource(SIRenderState);
    SICubeRenderState *cubes = &render->cubes;
    SIWindowRenderState *state = &render->windows;

    for (uint32_t i = 0; i < state->depth_target_count; i++) {
        SIDepthTarget *target = &state->depth_targets[i];
        if (target->window != window) {
            continue;
        }

        if (target->width == width && target->height == height && target->texture != NULL) {
            return target->texture;
        }

        if (target->texture != NULL) {
            SDL_ReleaseGPUTexture(engine->primary_gpu, target->texture);
        }
        target->texture = NULL;
        target->width = width;
        target->height = height;

        target->texture = SDL_CreateGPUTexture(
            engine->primary_gpu,
            &(SDL_GPUTextureCreateInfo){
                .type = SDL_GPU_TEXTURETYPE_2D,
                .format = cubes->depth_format,
                .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
                .width = width,
                .height = height,
                .layer_count_or_depth = 1,
                .num_levels = 1,
                .sample_count = SDL_GPU_SAMPLECOUNT_1,
            }
        );
        return target->texture;
    }

    if (state->depth_target_count >= 8) {
        fprintf(stderr, "siengine: maximum number of windows with depth targets reached\n");
        return NULL;
    }

    SIDepthTarget *target = &state->depth_targets[state->depth_target_count++];
    target->window = window;
    target->width = width;
    target->height = height;
    target->texture = SDL_CreateGPUTexture(
        engine->primary_gpu,
        &(SDL_GPUTextureCreateInfo){
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = cubes->depth_format,
            .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
            .width = width,
            .height = height,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
        }
    );

    if (target->texture == NULL) {
        fprintf(stderr, "siengine: SDL_CreateGPUTexture(depth) failed: %s\n", SDL_GetError());
    }

    return target->texture;
}

void siwindow_render_state_shutdown() {
    SIEngineCtx *engine = ecs_resource(SIEngineCtx);
    SIWindowRenderState *state = &ecs_resource(SIRenderState)->windows;

    for (uint32_t i = 0; i < state->depth_target_count; i++) {
        if (state->depth_targets[i].texture != NULL) {
            SDL_ReleaseGPUTexture(engine->primary_gpu, state->depth_targets[i].texture);
            state->depth_targets[i].texture = NULL;
        }
    }
    state->depth_target_count = 0;
}
