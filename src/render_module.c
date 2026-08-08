#include "render_internal.h"
#include <stdlib.h>

ECS_RESOURCE_DEFINE(SIRenderState);

void sirender_shutdown() {
    SIRenderState *render = ecs_try_get_resource(SIRenderState);
    if (!render)
        return;

    ecs_query_fini(render->camera_query);
    ecs_query_fini(render->sprite_query);
    SIEngineCtx *engine = ecs_try_get_resource(SIEngineCtx);
    if (engine && engine->primary_gpu) {
        for (uint32_t i = 0; i < 3; i++) {
            if (render->pipelines[i])
                SDL_ReleaseGPUGraphicsPipeline(engine->primary_gpu, render->pipelines[i]);
        }
        if (render->vertex_shader)
            SDL_ReleaseGPUShader(engine->primary_gpu, render->vertex_shader);
        if (render->fragment_shader)
            SDL_ReleaseGPUShader(engine->primary_gpu, render->fragment_shader);
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
    render->sprite_query = ecs_query({
        .terms = {
            ecs_in(SISprite),
            ecs_in(SIWorldTransform2D),
            ecs_in(SIColor),
            ecs_in(SISpriteFlip),
            ecs_in(SIPivot),
            ecs_in(SIBlendMode),
            ecs_in_optional(SISpriteSheet),
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
    render->extract_system = ecs_system(
        {
            .name = "ExtractRender",
            .phase = EcsOnRender,
            .callback = sirender_extract,
        }
    );
    ecs_system(
        {
            .name = "DrawWindow",
            .phase = EcsOnRender,
            .callback = sirender_draw_window,
            .after = { render->extract_system },
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
