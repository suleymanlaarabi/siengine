#include "render_internal.h"
#include <stdlib.h>

ECS_RESOURCE_DEFINE(SIRenderState);

void sirender_shutdown() {
    SIRenderState *render = ecs_try_get_resource(SIRenderState);
    if (!render)
        return;

    ecs_query_fini(render->camera_query);
    ecs_query_fini(render->renderable_query);
    SIEngineCtx *engine = ecs_try_get_resource(SIEngineCtx);
    if (engine && engine->primary_gpu) {
        for (uint32_t i = 0; i < 3; i++) {
            if (render->sprite_pipelines[i])
                SDL_ReleaseGPUGraphicsPipeline(engine->primary_gpu, render->sprite_pipelines[i]);
            if (render->shape_pipelines[i])
                SDL_ReleaseGPUGraphicsPipeline(engine->primary_gpu, render->shape_pipelines[i]);
            if (render->circle_pipelines[i])
                SDL_ReleaseGPUGraphicsPipeline(engine->primary_gpu, render->circle_pipelines[i]);
        }
        if (render->vertex_shader)
            SDL_ReleaseGPUShader(engine->primary_gpu, render->vertex_shader);
        if (render->sprite_fragment_shader)
            SDL_ReleaseGPUShader(engine->primary_gpu, render->sprite_fragment_shader);
        if (render->shape_fragment_shader)
            SDL_ReleaseGPUShader(engine->primary_gpu, render->shape_fragment_shader);
        if (render->circle_fragment_shader)
            SDL_ReleaseGPUShader(engine->primary_gpu, render->circle_fragment_shader);
        for (uint32_t i = 0; i < 2; i++) {
            if (render->samplers[i])
                SDL_ReleaseGPUSampler(engine->primary_gpu, render->samplers[i]);
        }
        if (render->vertex_buffer)
            SDL_ReleaseGPUBuffer(engine->primary_gpu, render->vertex_buffer);
        if (render->transfer_buffer)
            SDL_ReleaseGPUTransferBuffer(engine->primary_gpu, render->transfer_buffer);
    }
    for (uint32_t i = 0; i < render->view_capacity; i++)
        free(render->views[i].queue.commands);
    free(render->views);
    free(render->vertices);
    render->views = NULL;
    render->vertices = NULL;
    render->cmd = NULL;
}

void sirender_register() {
    ECS_RESOURCE_REGISTER(SIRenderState);
    ecs_set_resource(SIRenderState, {});

    SIRenderState *render = ecs_resource(SIRenderState);
    render->camera_query = ecs_query({
        .terms = {
            ecs_in(SICamera2D),
            ecs_in(SIWorldTransform2D),
            ecs_in(SICameraViewport),
            ecs_in_optional(SIVirtualResolution),
        },
    });
    render->renderable_query = ecs_query({
        .terms = {
            ecs_filter(SIRenderable),
            ecs_in(SIWorldTransform2D),
            ecs_in(SIColor),
            ecs_in(SIBlendMode),
            ecs_in_optional(SISprite),
            ecs_in_optional(SISpriteFlip),
            ecs_in_optional(SIPivot),
            ecs_in_optional(SISpriteSheet),
            ecs_in_optional(SICircle),
            ecs_in_optional(SIRectangle),
            ecs_in_optional(SITriangle),
        },
        .relations = {
            ecs_rel(Layer),
        },
        .order_by = ecs_order_by_target(Layer),
    });

    ecs_system(
        {
            .name = "BeginFrame",
            .phase = EcsPreRender,
            .callback = sirender_begin_frame,
        }
    );
    ecs_system(
        {
            .name = "ExtractRender",
            .phase = EcsPreRender,
            .callback = sirender_extract,
        }
    );
    ecs_system(
        {
            .name = "DrawWindow",
            .phase = EcsOnRender,
            .callback = sirender_draw_window,
        }
    );
    ecs_system(
        {
            .name = "EndFrame",
            .phase = EcsPostRender,
            .callback = sirender_end_frame,
        }
    );
}
