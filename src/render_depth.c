#include "render_internal.h"

SDL_GPUTexture *siwindow_ensure_depth_target(uint32_t width, uint32_t height) {
    SIEngineCtx *engine = ecs_resource(SIEngineCtx);
    SIRenderState *render = ecs_resource(SIRenderState);
    SICubeRenderState *cubes = &render->cubes;
    SIDepthTarget *target = &render->windows.depth_target;

    if (target->width == width && target->height == height && target->texture != NULL)
        return target->texture;

    if (target->texture != NULL)
        SDL_ReleaseGPUTexture(engine->primary_gpu, target->texture);

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

void siwindow_render_state_shutdown() {
    SIEngineCtx *engine = ecs_resource(SIEngineCtx);
    SIDepthTarget *target = &ecs_resource(SIRenderState)->windows.depth_target;

    if (target->texture != NULL) {
        SDL_ReleaseGPUTexture(engine->primary_gpu, target->texture);
        target->texture = NULL;
    }
    target->width = 0;
    target->height = 0;
}
